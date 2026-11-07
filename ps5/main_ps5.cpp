// rotk-ps5: the app's entry on a PS5. Finds the player's copy of The Return of the King (US Xbox), checks it is the
// release this build was recompiled from, and boots it: the disc image mounted as the D: drive, as the PC port does.
// The boot order follows the PC port's (runtime/app/main.cpp) and the harness's main_test.cpp; the console parts
// (affinity, splash, notifications, folders) follow reUndr's ps5/src/main_ps5.c (MIT, bystritskiy).
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

#include "app/app.h"
#include "core/input.h"
#include "core/log.h"
#include "image_symbols.h"
#include "kernel/disc.h"
#include "kernel/kernel.h"
#include "ps5/ps5_host.h"
#include "ps5/xbe_match.h"
#include "xbe/image_hash.h"

extern const uint64_t kImageSectionsHash;  // written by xbe_recomp from the default.xbe this build was made from
namespace ps5 {  // written by tools/xbe_blocks.cpp from the same default.xbe (ps5/xbe_match.h)
extern const uint64_t kXbeLayout;
extern const uint32_t kXbeBlockCount;
extern const uint64_t kXbeBlocks[];
}  // namespace ps5
namespace rotk {
extern std::filesystem::path g_ini_file;  // the port's rotk_settings.cpp: ROTK PC's Data folder is beside it
}

extern "C" {
int sceSystemServiceHideSplashScreen(void);
int sceKernelSendNotificationRequest(int, void*, size_t, int);
int scePthreadSetaffinity(unsigned long, unsigned long);
int sceKernelDebugOutText(int, const char*);
}

namespace ps5 {

const std::filesystem::path& DataDir() {
    static const std::filesystem::path d = "/data/rotk";
    return d;
}

void Notify(const char* text) {
    struct {
        char reserved[45];
        char message[3075];
    } note{};
    std::snprintf(note.message, sizeof note.message, "%s", text);
    sceKernelSendNotificationRequest(0, &note, sizeof note, 0);
}

namespace {

bool IsFile(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::is_regular_file(p, ec);
}

bool ImageIn(const std::filesystem::path& dir, Game& game) {
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
        std::string n = e.path().filename().string();
        if (n.size() < 5 || n[0] == '.') continue;
        std::string ext = n.substr(n.size() - 4);
        for (char& c : ext) c = char(std::tolower(c));
        if (ext != ".iso" || !e.is_regular_file(ec)) continue;
        game = {e.path(), true};
        return true;
    }
    return false;
}

bool In(const std::filesystem::path& dir, Game& game) {
    if (ImageIn(dir, game)) return true;
    if (IsFile(dir / "game" / IMAGE_XBE)) {
        game = {dir / "game", false};
        return true;
    }
    return false;
}

}  // namespace

bool FindGame(Game& game) {
    if (In(DataDir(), game)) return true;
    for (int i = 0; i < 8; i++)
        if (In("/mnt/usb" + std::to_string(i) + "/rotk", game)) return true;
    return false;
}

}  // namespace ps5

namespace {

// the image's default.xbe, read from the disc image or the folder
bool ReadXbe(const ps5::Game& g, std::unique_ptr<kernel::Disc>& disc, std::vector<uint8_t>& xbe, std::string& error) {
    if (g.image) {
        disc = std::make_unique<kernel::Disc>();
        if (!disc->Open(g.path, error)) return false;
        if (!disc->Load(IMAGE_XBE, xbe)) {
            error = "no " IMAGE_XBE " on the disc image";
            return false;
        }
        return true;
    }
    std::ifstream f(g.path / IMAGE_XBE, std::ios::binary);
    xbe.assign(std::istreambuf_iterator<char>(f), {});
    if (xbe.empty()) error = "could not read " IMAGE_XBE;
    return !xbe.empty();
}

uint32_t TitleId(const std::vector<uint8_t>& x) {
    auto u32 = [&](size_t at) -> uint32_t {
        uint32_t v = 0;
        if (at + 4 <= x.size()) std::memcpy(&v, x.data() + at, 4);
        return v;
    };
    if (x.size() < 0x180 || std::memcmp(x.data(), "XBEH", 4)) return 0;
    uint32_t base = u32(0x104), cert = u32(0x118);
    return cert >= base ? u32(cert - base + 8) : 0;
}

// a pipe's read end drained into a file by a thread of its own (reUndr's open_log, main_ps5.c)
struct Sink {
    int from, to;
};
void* Drain(void* p) {
    auto* s = static_cast<Sink*>(p);
    char buf[16384];
    for (;;) {
        ssize_t n = read(s->from, buf, sizeof buf);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        if (s->to < 0) continue;  // no log: the lines are dropped
        for (ssize_t done = 0; done < n;) {
            ssize_t w = write(s->to, buf + done, size_t(n - done));
            if (w < 0 && errno == EINTR) continue;
            if (w <= 0) break;
            done += w;
        }
    }
    close(s->from);
    if (s->to >= 0) close(s->to);
    return nullptr;
}
// path null: the lines are read and dropped, so nothing that writes to stderr ever waits
FILE* OpenLog(const char* path) {
    int fd = -1;
    if (path && (fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) return nullptr;
    int ends[2];
    if (pipe(ends) == 0) {
        static Sink sink;
        sink = {ends[0], fd};
        pthread_t t;
        if (FILE* stream = fdopen(ends[1], "w"); stream && pthread_create(&t, nullptr, Drain, &sink) == 0) {
            pthread_detach(t);
            std::setvbuf(stream, nullptr, _IOLBF, 0);  // a line at a time into the pipe: memory, not the disk
            return stream;
        }
        close(ends[0]);
        close(ends[1]);
    }
    return fd >= 0 ? fdopen(fd, "w") : nullptr;
}

[[noreturn]] void Stop(const char* why) {
    LOGE("%s", why);
    logx::Flush();
    ps5::Notify(why);
    sceKernelDebugOutText(0, "[rotk] stopped\n");
    _exit(0);
}

}  // namespace

int main() {
    scePthreadSetaffinity((unsigned long)pthread_self(), 0x1fff);  // the cores a title may use
    const auto& data = ps5::DataDir();
    for (const char* sub : {"", "/logs", "/hdd", "/shadercache", "/textures"}) {
        std::string d = data.string() + sub;
        mkdir(d.c_str(), 0777);
        chmod(d.c_str(), 0777);
    }
    // stderr (Mesa, the C runtime, and a copy of every log line) into a file beside the game's own log. The lines go
    // into a pipe a thread of its own drains to the file: the console ends a crashing app at once, and the file still
    // holds nearly everything up to that moment, while the game never waits on the disk (0.1.1 wrote each line
    // straight to the file, 40-65 ms a line on the game's thread: its hiccups). dup2 onto fd 2 isn't seen by the
    // PS5 libc's stderr, so the stream itself is replaced, as reUndr's main_ps5.c does.
    // Logging is for finding a problem: on only while the file /data/rotk/logging-on exists. Off, no line is
    // formatted or written, and stderr's (Mesa's own) are dropped in the pipe.
    std::error_code ec;
    const bool logging = std::filesystem::exists(data / "logging-on", ec);
    if (!logging) logx::Off();
    if (FILE* err = OpenLog(logging ? (data / "logs/stderr.log").c_str() : nullptr)) stderr = err;
    // Mesa's shader cache: compiled GPU code kept between runs, in a folder the app can write
    setenv("MESA_SHADER_CACHE_DIR", (data / "shadercache/mesa").c_str(), 0);
    setenv("XDG_CACHE_HOME", (data / "shadercache").c_str(), 0);
    logx::Init(app::LogFolder().string(), app::kGame.log);
    LOGI("rotk-ps5: %s", ROTK_PS5_VERSION);
    app::Load();
    app::LogSystem();

    ps5::Game game;
    if (!ps5::FindGame(game))
        Stop("ROTK: game not found. Copy your disc image (.iso) of the US Xbox release to /data/rotk/ or to "
             "rotk/ on a USB drive.");
    LOGI("game: %s (%s)", game.path.c_str(), game.image ? "disc image" : "files");
    std::unique_ptr<kernel::Disc> disc;
    std::vector<uint8_t> xbe;
    std::string error;
    if (!ReadXbe(game, disc, xbe, error)) Stop(("ROTK: the disc image could not be read: " + error).c_str());
    if (TitleId(xbe) != 0x4541003E) Stop("ROTK: that disc is another game. It needs The Return of the King (US Xbox).");
    // the same bytes as the build's default.xbe, or the same release with a few bytes of code changed: a
    // media-patched dump, or a clean one (ps5/xbe_match.h)
    uint32_t differ = 0;
    if (xbe::SectionsHash(xbe) != kImageSectionsHash &&
        !ps5::xbe_match::SameRelease(xbe, ps5::kXbeLayout, ps5::kXbeBlocks, ps5::kXbeBlockCount, differ))
        Stop("ROTK: this is another release of the game (not the US one this build was made from).");
    LOGI("game: %s is the release this build was made from%s", IMAGE_XBE,
         differ ? " (a few bytes of code differ: a media-patched or clean dump)" : "");

    sceSystemServiceHideSplashScreen();
    ImGui::CreateContext();  // the renderer's overlay: the in-game menu and the fps counter, as the PC port's main.cpp
    ImGui::GetIO().IniFilename = nullptr;
    app::OverlayInit();
    input::Init();
    if (app::kGame.init) app::kGame.init();
    // ROTK PC's Data folder, when the player put their copy's in /data/rotk/pc: sharper text reads its larger fonts
    // and menu pictures (rotk_fonts.cpp, rotk_menu_art.cpp look beside the PC game's ROTKFix.ini, which needn't exist)
    {
        if (std::filesystem::is_regular_file(data / "pc/Data/Game/GlobScen.scw", ec)) {
            rotk::g_ini_file = data / "pc/ROTKFix.ini";
            LOGI("rotk: ROTK PC's Data folder in %s (sharper text and menu pictures)", (data / "pc").c_str());
        }
    }
    std::string root = game.image ? game.path.parent_path().string() : game.path.string();
    if (disc) kernel::MountDisc(std::move(disc));
    if (!kernel::Boot(std::move(xbe), IMAGE_XBE, root, app::SavesDir().string())) Stop("ROTK: the game could not start.");
    for (;;) {
        sleep(1);
        logx::Flush();
    }
}
