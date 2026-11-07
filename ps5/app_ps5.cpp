// rotk-ps5: the host side on a PS5, in place of runtime/app/ (Windows) and the harness's app_headless.cpp.
// Everything the port writes lives in /data/rotk: settings.toml, saves (hdd), logs, the shader cache, screenshots and
// texture packs. The picture goes out at the console's video mode (4K by default), drawn at render_percent of it.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "app/app.h"
#include "core/log.h"
#include "ps5/ps5_host.h"

namespace rotk {
extern bool g_skip_all_videos, g_skip_logos;  // the port's src/game/rotk_movies.cpp
extern bool g_sharp_text;                     // rotk_fonts.cpp: menu art upscaled, text from ROTK PC's fonts
}

namespace app {

Settings& Get() {
    static Settings s;
    return s;
}
void Load() {
    // the PS5's defaults where they differ from the PC's: every video can be skipped with a button press (on the
    // console a story movie only after it has been seen once), and the EA / New Line logos are passed over
    rotk::g_skip_all_videos = true;
    rotk::g_skip_logos = true;
    // sharper menus: the title, main menu, save and options pictures upscaled four times on a 4K screen (and the
    // text redrawn from ROTK PC's larger fonts when the player puts that game's Data folder in /data/rotk/pc)
    rotk::g_sharp_text = true;
    std::error_code ec;
    const bool first = !std::filesystem::exists(SettingsFile(), ec);
    std::string error;
    if (!cfg::Read(SettingsFile().string(), SettingsList(), error)) LOGE("settings: %s", error.c_str());
    // sharp_text became the PS5's default in 0.1.3; a settings file written before then holds it off, so it is turned
    // on once (the marker file records that it was), and from then on the menu's choice stands
    const std::filesystem::path mark = ps5::DataDir() / "defaults-0.1.6";
    if (!std::filesystem::exists(mark, ec)) {
        if (!first && !rotk::g_sharp_text) {
            rotk::g_sharp_text = true;
            LOGI("settings: sharp_text turned on (the PS5's default since 0.1.3)");
            if (!cfg::Write(SettingsFile().string(), SettingsList())) LOGE("settings: could not write %s", SettingsFile().c_str());
        }
        if (FILE* f = std::fopen(mark.c_str(), "w")) std::fclose(f);
    }
    Settings& s = Get();
    s.renderer = 1;                  // Vulkan: the only renderer here
    s.display_mode = DisplayMode::Fullscreen;
    s.widescreen = true;             // every PS5 is on a 16:9 screen
    s.keyboard = false;
    s.discord = false;
    s.texture_dump = false;          // a tool for making this port, never on in play
    if (first) {  // the first start writes every setting with its default, to edit over FTP
        Save();
        LOGI("settings: %s made with the defaults", SettingsFile().c_str());
    }
}
void Save() {
    if (!cfg::Write(SettingsFile().string(), SettingsList())) LOGE("settings: could not write %s", SettingsFile().c_str());
}
void LogSettings() {}
void LogSystem() { LOGI("system: PS5, output %ux%u", Output().width, Output().height); }
uint64_t OneCoreMask() { return 1; }

// [display] width/height pick the output (1920x1080, 2560x1440 or the default 3840x2160)
Mode Desktop() { return {3840, 2160}; }
Mode Output() {
    const Settings& s = Get();
    if (s.height == 1080 || s.height == 1440) return {s.height * 16 / 9, s.height};
    return Desktop();
}
// the console drew 480 lines: drawn at render_percent of the output's
float RenderScale(uint32_t guest_height) {
    if (!guest_height) return 1.0f;
    uint32_t pct = std::clamp(Get().render_percent, 25u, 200u);
    return std::max(1.0f, float(Output().height) * float(pct) / 100.0f / float(guest_height));
}
bool Widescreen() { return true; }
float g_max_frame_aspect = 16.0f / 9.0f;  // set by a game that handles wider screens; a TV is 16:9 anyway
float FrameAspect() { return 16.0f / 9.0f; }

std::filesystem::path ExeDir() { return ps5::DataDir(); }
int Instance() { return 0; }
void* Window() { return nullptr; }

std::filesystem::path TextureDir() { return ps5::DataDir() / "textures"; }
std::filesystem::path SavesDir() { return ps5::DataDir() / "hdd"; }
std::filesystem::path ShaderCacheDir() { return ps5::DataDir() / "shadercache"; }
std::filesystem::path ScreenshotDir() { return ps5::DataDir() / "Screenshots"; }
std::filesystem::path LogFolder() { return ps5::DataDir() / "logs"; }
std::filesystem::path SettingsFile() { return ps5::DataDir() / "settings.toml"; }
const std::vector<std::string>& ScreenNames() {
    static const std::vector<std::string> n{"main"};
    return n;
}
void* ScreenMonitor() { return nullptr; }

const CommandLine& Args() {
    static const CommandLine c = [] {
        CommandLine c;
        c.no_dialogs = true;
        return c;
    }();
    return c;
}

void Fail(int code, const std::wstring& message) {
    std::string m(message.begin(), message.end());
    LOGE("Exit (%d): %s", code, m.c_str());
    logx::Flush();
    ps5::Notify(("ROTK: " + m).c_str());
    std::_Exit(code);
}

// no splash, window or handover on a console
bool OpenSplash() { return false; }
void SplashFirstFrame() {}
void CloseSplash() {}
void OpenGameWindow(bool) {}
bool StartHandover() { return false; }
bool HandingOver() { return false; }
void FinishHandover() {}

std::recursive_mutex& UiLock() {
    static std::recursive_mutex m;
    return m;
}

// the in-game menu, its standstill and UiScale: ps5/overlay_ps5.cpp

void Subtitle(const std::string& utf8, double seconds) {
    static std::string last;
    if (utf8 == last) return;
    last = utf8;
    LOGI("subtitle (%.0f s): %s", seconds, utf8.c_str());
}
void DrawSubtitles(float) {}
void MovieSubtitles(const std::string&) {}

// fps_cap (the game's frame rate setting) as the window does; vsync paces the rest
void Pace() {
    using clock = std::chrono::steady_clock;
    static clock::time_point next = clock::now();
    uint32_t cap = Get().fps_cap;
    auto now = clock::now();
    if (!cap) {
        next = now;
        return;
    }
    auto period = std::chrono::nanoseconds(1000000000ull / cap);
    next += period;
    if (next < now - period) next = now;
    std::this_thread::sleep_until(next);
}

}  // namespace app

// achievements are counted and kept with the saves (/data/rotk/hdd/achievements.txt) in a later build; logged for now
namespace ach {
void Start() {}
void Unlock(const char* id) {
    static std::map<std::string, bool> seen;
    if (!seen[id]) LOGR("achievements: earned %s", id);
    seen[id] = true;
}
bool Has(const char*) { return false; }
static std::map<std::string, uint64_t>& Counters() {
    static std::map<std::string, uint64_t> m;
    return m;
}
uint64_t Count(const char* n) { return Counters()[n]; }
uint64_t Add(const char* n, uint64_t v) { return Counters()[n] += v; }
void Flush() {}
void DrawNotes(float) {}
void DrawList() {}
uint32_t VirtueDealt() { return 100; }
uint32_t VirtueTaken() { return 100; }
bool Fate() { return false; }
bool FellowshipOn() { return false; }
void SharedDeeds(std::vector<Def>&) {}
void Defeated() {}
void LevelStarted(const char*) {}
void LevelDone(const char*) {}
bool FateDone(const char*) { return false; }
}  // namespace ach
