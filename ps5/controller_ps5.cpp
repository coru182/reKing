// rotk-ps5: the help screen shown while a level loads (after character select) draws a PlayStation-layout controller
// where the Xbox game shows its Xbox controller: ps5/art/controller.png, made by tools/mkcontroller.py from
// ps5/art/controller.svg.
//
// That screen's controller is one of the game's full-screen pictures: 512x448, YUY2, linear (format 0x24), stretched
// over the whole 16:9 screen, with the game's own labels, arrows and text drawn over it (rotk_menu_art.cpp handles
// the other such pictures). It is recognised by what it shows: a mostly dark picture holding four coloured face
// buttons in their diamond, yellow above, blue left, red right, green below. Ours is placed by where they are found
// (measured once from a screenshot: the face buttons centred at texel 267.5, 138.5, 32.5 texels from blue to red and
// 38.9 from yellow to green), so its face buttons land on the Xbox ones and the game's labels still point at them.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>

#include "core/log.h"
#include "gpu/backend.h"
#include "gpu/texture_pack.h"

extern const unsigned char kControllerPng[];  // ps5/art/controller.png, built in (build-ps5.sh)
extern const unsigned int kControllerPngSize;

namespace rotk::ps5art {
namespace {

constexpr uint32_t kArtScale = 4;  // controller.png is the 512x448 picture four times larger
constexpr float kCx = 267.45f, kCy = 138.48f, kSpanX = 32.51f, kSpanY = 38.91f;

struct Rgb {
    uint8_t r, g, b;
};

std::vector<Rgb> FromYuy2(const uint8_t* p, uint32_t w, uint32_t h, uint32_t pitch) {
    std::vector<Rgb> rgb(size_t(w) * h);
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) {
            const uint8_t* q = p + size_t(y) * pitch + (x & ~1u) * 2;
            float Y = q[(x & 1) * 2], U = q[1] - 128.0f, V = q[3] - 128.0f;
            auto c = [](float v) { return uint8_t(std::clamp(v, 0.0f, 255.0f) + 0.5f); };
            rgb[size_t(y) * w + x] = {c(Y + 1.402f * V), c(Y - 0.344f * U - 0.714f * V), c(Y + 1.772f * U)};
        }
    return rgb;
}

enum Colour : uint8_t { kNone, kYellow, kBlue, kRed, kGreen };

Colour Classify(Rgb c) {
    int r = c.r, g = c.g, b = c.b;
    if (r > 150 && g > 100 && b < 90 && r > g + 20) return kYellow;
    if (b > 110 && b > r + 70 && b > g + 70) return kBlue;
    if (r > 100 && r > g + 55 && r > b + 55) return kRed;
    if (g > 90 && g > r + 55 && g > b + 55) return kGreen;
    return kNone;
}

struct Blob {
    Colour colour;
    float x, y;
    uint32_t area;
};

// the patches of each colour (8-connected), small ones only: buttons, not scenery
std::vector<Blob> Blobs(const std::vector<Colour>& cls, uint32_t w, uint32_t h) {
    std::vector<Blob> out;
    std::vector<uint8_t> seen(cls.size());
    std::vector<uint32_t> stack;
    for (uint32_t i = 0; i < cls.size(); i++) {
        if (cls[i] == kNone || seen[i]) continue;
        Colour c = cls[i];
        double sx = 0, sy = 0;
        uint32_t n = 0;
        stack.assign(1, i);
        seen[i] = 1;
        while (!stack.empty()) {
            uint32_t k = stack.back();
            stack.pop_back();
            uint32_t x = k % w, y = k / w;
            sx += x, sy += y, n++;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = int(x) + dx, ny = int(y) + dy;
                    if (nx < 0 || ny < 0 || nx >= int(w) || ny >= int(h)) continue;
                    uint32_t j = uint32_t(ny) * w + uint32_t(nx);
                    if (!seen[j] && cls[j] == c) seen[j] = 1, stack.push_back(j);
                }
        }
        if (n >= 8 && n <= 1200) out.push_back({c, float(sx / n), float(sy / n), n});
    }
    return out;
}

struct Diamond {
    float cx, cy, span_x, span_y, error;
};

// the four face buttons in their diamond: yellow above green, blue left of red, around one centre
bool FindDiamond(const std::vector<Blob>& blobs, Diamond& best) {
    best.error = 1e9f;
    for (const Blob& y : blobs) {
        if (y.colour != kYellow) continue;
        for (const Blob& a : blobs) {
            if (a.colour != kGreen) continue;
            float sy = a.y - y.y;
            if (sy < 10 || sy > 160) continue;
            for (const Blob& x : blobs) {
                if (x.colour != kBlue) continue;
                for (const Blob& b : blobs) {
                    if (b.colour != kRed) continue;
                    float sx = b.x - x.x;
                    if (sx < 8 || sx > 140 || sy / sx < 0.85f || sy / sx > 1.6f) continue;
                    float off_v = std::fabs(y.x - a.x) / sx, off_h = std::fabs(x.y - b.y) / sy;
                    float cx1 = (y.x + a.x) / 2, cy1 = (y.y + a.y) / 2, cx2 = (x.x + b.x) / 2, cy2 = (x.y + b.y) / 2;
                    float apart = std::hypot(cx1 - cx2, cy1 - cy2) / sx;
                    if (off_v > 0.25f || off_h > 0.25f || apart > 0.25f) continue;
                    float error = off_v + off_h + apart;
                    if (error < best.error) best = {(cx1 + cx2) / 2, (cy1 + cy2) / 2, sx, sy, error};
                }
            }
        }
    }
    return best.error < 1e9f;
}

std::once_flag g_decoded;
gpu::pack::Image g_art;  // controller.png, RGBA8, level 0 only

std::mutex g_lock;
uint64_t g_last_hash = 0;
gpu::pack::Image g_last;  // the last picture made, for the same contents again

uint64_t Hash(const uint8_t* p, size_t n) {
    uint64_t h = 0xCBF29CE484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001B3ull;
    return h;
}

}  // namespace

// a texture the game made: the help screen's Xbox controller drawn as a PlayStation one, true with it in out
bool Controller(const gpu::pack::Contents& t, gpu::pack::Image& out) {
    if (t.format != 0x24 || !t.data || t.width < 256 || t.height < 224 || t.width > 1024 || t.height > 1024 ||
        gpu::g_movie)
        return false;
    uint32_t w = t.width, h = t.height, pitch = uint32_t(t.size / h);
    if (pitch < w * 2) return false;
    uint64_t hash = Hash(t.data, size_t(pitch) * h);
    {
        std::lock_guard<std::mutex> g(g_lock);
        if (hash == g_last_hash && !g_last.levels.empty()) {
            out = g_last;
            return true;
        }
    }
    std::vector<Rgb> rgb = FromYuy2(t.data, w, h, pitch);
    // mostly dark: the help screen's picture is the controller on black
    size_t dark = 0;
    for (const Rgb& c : rgb) dark += (c.r * 3 + c.g * 6 + c.b) < 400;  // luma under 40
    if (dark * 100 < rgb.size() * 35) return false;
    std::vector<Colour> cls(rgb.size());
    for (size_t i = 0; i < rgb.size(); i++) cls[i] = Classify(rgb[i]);
    std::vector<Blob> blobs = Blobs(cls, w, h);
    Diamond d;
    if (!FindDiamond(blobs, d)) {
        LOG_FIRST(8, "ps5 controller: a dark %ux%u picture without the face buttons' diamond (%zu coloured patches)", w, h,
                  blobs.size());
        return false;
    }
    // the picture's texels per art texel, from the face buttons; a picture far from the screenshot's is left alone
    float kx = d.span_x / kSpanX, ky = d.span_y / kSpanY;
    float scale_w = float(w) / 512.0f, scale_h = float(h) / 448.0f;
    if (kx < 0.6f * scale_w || kx > 1.5f * scale_w || ky < 0.6f * scale_h || ky > 1.5f * scale_h) {
        LOG_FIRST(8, "ps5 controller: face buttons at %.1f, %.1f, %.1f x %.1f apart: not the size expected", d.cx, d.cy,
                  d.span_x, d.span_y);
        return false;
    }
    std::call_once(g_decoded, [] {
        gpu::pack::Image img;
        if (gpu::pack::DecodePng(kControllerPng, kControllerPngSize, img) && !img.levels.empty()) {
            img.levels.resize(1);
            g_art = std::move(img);
        }
        LOGI("ps5 controller: the PlayStation controller picture %s (%ux%u)", g_art.levels.empty() ? "did not decode" : "ready",
             g_art.width, g_art.height);
    });
    if (g_art.levels.empty()) return false;

    // the picture made kArtScale times larger: each texel's place in the art, found through the face buttons
    const uint32_t W = w * kArtScale, H = h * kArtScale, AW = g_art.width, AH = g_art.height;
    const std::vector<uint8_t>& art = g_art.levels[0];
    gpu::pack::Image img;
    img.width = W;
    img.height = H;
    img.format = gpu::pack::Format::RGBA8;
    std::vector<uint8_t>& px = img.levels.emplace_back(size_t(W) * H * 4);
    for (uint32_t v = 0; v < H; v++)
        for (uint32_t u = 0; u < W; u++) {
            float x = (u + 0.5f) / kArtScale, y = (v + 0.5f) / kArtScale;     // the game's texels
            float ax = (kCx + (x - d.cx) / kx) * kArtScale - 0.5f;             // the art's
            float ay = (kCy + (y - d.cy) / ky) * kArtScale - 0.5f;
            uint8_t* to = &px[(size_t(v) * W + u) * 4];
            to[3] = 255;
            if (ax < 0 || ay < 0 || ax > AW - 1.0f || ay > AH - 1.0f) {
                to[0] = to[1] = to[2] = 0;
                continue;
            }
            uint32_t x0 = std::min(uint32_t(ax), AW - 2), y0 = std::min(uint32_t(ay), AH - 2);
            float fx = ax - x0, fy = ay - y0;
            const uint8_t* a = &art[(size_t(y0) * AW + x0) * 4];
            const uint8_t* b = a + size_t(AW) * 4;
            for (int k = 0; k < 3; k++) {
                float top = a[k] * (1 - fx) + a[4 + k] * fx, bottom = b[k] * (1 - fx) + b[4 + k] * fx;
                to[k] = uint8_t(top * (1 - fy) + bottom * fy + 0.5f);
            }
        }
    gpu::pack::MakeLevels(img);
    LOGI("ps5 controller: the help screen's Xbox controller (face buttons at %.1f, %.1f, %.1f x %.1f apart) drawn as a "
         "PlayStation one",
         d.cx, d.cy, d.span_x, d.span_y);
    {
        std::lock_guard<std::mutex> g(g_lock);
        g_last_hash = hash;
        g_last = img;
    }
    out = std::move(img);
    return true;
}

}  // namespace rotk::ps5art
