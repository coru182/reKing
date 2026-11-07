// rotk-ps5: sharper text for every font the game draws, with or without ROTK PC's fonts.
//
// The game's fonts are 4-bit coverage atlases in P8 textures, one colour per coverage level (rotk_fonts.cpp). With
// ROTK PC's Data folder, rotk_fonts.cpp redraws the ones it knows from the PC's larger glyphs. What that leaves (every
// font without the PC folder, a font it does not know, such as the title face of the in-game dialogs, and glyphs the
// PC font lacks) used to be the Xbox atlas scaled up smoothly: soft and blurry on a 4K screen. Here such an atlas is
// made four times larger with crisp edges instead: the coverage is enlarged with a Catmull-Rom filter, which keeps
// the letters' curves, and then each edge is redrawn one output pixel wide where the enlarged coverage crosses half
// (its distance from that line found from the slope). Thin strokes that never reach half coverage keep their own,
// lower line, so hairlines stay.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "core/log.h"
#include "gpu/texture_pack.h"

namespace rotk::ps5art {

// coverage 0..15, w x h, made k times larger with crisp edges, 0..15
std::vector<float> Crisp(const uint8_t* cov, uint32_t w, uint32_t h, uint32_t k) {
    const uint32_t W = w * k, H = h * k;
    // Catmull-Rom weights for the four source texels around each output column / row
    struct Taps {
        int first;
        float w[4];
    };
    auto taps = [&](uint32_t n_out) {
        std::vector<Taps> t(n_out);
        for (uint32_t u = 0; u < n_out; u++) {
            float x = (u + 0.5f) / float(k) - 0.5f, f = std::floor(x), a = x - f, a2 = a * a, a3 = a2 * a;
            t[u].first = int(f) - 1;
            t[u].w[0] = (-a3 + 2 * a2 - a) / 2;
            t[u].w[1] = (3 * a3 - 5 * a2 + 2) / 2;
            t[u].w[2] = (-3 * a3 + 4 * a2 + a) / 2;
            t[u].w[3] = (a3 - a2) / 2;
        }
        return t;
    };
    std::vector<Taps> tx = taps(W), ty = taps(H);
    auto clampi = [](int v, int hi) { return std::clamp(v, 0, hi); };
    // across, then down; coverage as 0..1
    std::vector<float> across(size_t(W) * h);
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t u = 0; u < W; u++) {
            float v = 0;
            for (int i = 0; i < 4; i++) v += tx[u].w[i] * cov[size_t(y) * w + clampi(tx[u].first + i, int(w) - 1)];
            across[size_t(y) * W + u] = v / 15.0f;
        }
    std::vector<float> up(size_t(W) * H);
    for (uint32_t v = 0; v < H; v++)
        for (uint32_t u = 0; u < W; u++) {
            float s = 0;
            for (int i = 0; i < 4; i++) s += ty[v].w[i] * across[size_t(clampi(ty[v].first + i, int(h) - 1)) * W + u];
            up[size_t(v) * W + u] = s;
        }
    // the strongest coverage near each source texel (3 x 3): a faint stroke's own peak
    std::vector<float> peak(size_t(w) * h);
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) {
            uint8_t m = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    m = std::max(m, cov[size_t(clampi(int(y) + dy, int(h) - 1)) * w + clampi(int(x) + dx, int(w) - 1)]);
            peak[size_t(y) * w + x] = m / 15.0f;
        }
    std::vector<float> out(size_t(W) * H);
    for (uint32_t v = 0; v < H; v++)
        for (uint32_t u = 0; u < W; u++) {
            size_t i = size_t(v) * W + u;
            float c = up[i];
            // the slope, per output pixel
            float gx = (up[size_t(v) * W + std::min(u + 1, W - 1)] - up[size_t(v) * W + (u ? u - 1 : 0)]) * 0.5f;
            float gy = (up[size_t(std::min(v + 1, H - 1)) * W + u] - up[size_t(v ? v - 1 : 0) * W + u]) * 0.5f;
            float g = std::sqrt(gx * gx + gy * gy);
            float p = peak[size_t(v / k) * w + u / k];
            if (p <= 0.0f) {
                out[i] = 0;
                continue;
            }
            // the line drawn at half coverage, or at half a faint stroke's peak, which then keeps its faintness
            float line = p >= 0.5f ? 0.5f : 0.5f * p;
            float strength = p >= 0.5f ? 1.0f : p / 0.5f;
            float a = g > 1e-4f ? std::clamp((c - line) / g + 0.5f, 0.0f, 1.0f) : (c >= line ? 1.0f : 0.0f);
            out[i] = a * strength * 15.0f;
        }
    return out;
}

namespace {

uint32_t Log2(uint32_t v) {
    uint32_t n = 0;
    while ((1u << n) < v) n++;
    return n;
}

// Xbox swizzle (Morton order: x and y bits interleave from the lowest, an axis drops out once its size is used up)
void Masks(uint32_t w, uint32_t h, uint32_t& mx, uint32_t& my) {
    mx = my = 0;
    uint32_t bit = 1, lx = Log2(w), ly = Log2(h);
    for (uint32_t i = 0; i < lx || i < ly; i++) {
        if (i < lx) mx |= bit, bit <<= 1;
        if (i < ly) my |= bit, bit <<= 1;
    }
}
uint32_t Deposit(uint32_t v, uint32_t mask) {
    uint32_t r = 0;
    for (uint32_t bit = 1; mask; bit <<= 1) {
        uint32_t low = mask & (~mask + 1);
        if (v & bit) r |= low;
        mask &= mask - 1;
    }
    return r;
}

uint64_t Hash(const uint8_t* p, size_t n) {
    uint64_t h = 0xCBF29CE484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001B3ull;
    return h;
}

constexpr uint32_t kScale = 4;

std::mutex g_lock;
std::unordered_map<uint64_t, std::vector<float>> g_crisp;  // by the atlas's coverage: its crisp copy

}  // namespace

// a P8 texture the PC fonts did not replace: when it is a font atlas (coverage 0..15 in the indices, the palette a
// ramp of opacity), true with it four times larger and crisp in out
bool FontTexture(const gpu::pack::Contents& t, gpu::pack::Image& out) {
    if (t.format != 0x0B || !t.data || !t.palette) return false;
    uint32_t w = t.width, h = t.height;
    if (w < 128 || h < 128 || w > 1024 || h > 1024 || (w & (w - 1)) || (h & (h - 1)) || t.size < size_t(w) * h)
        return false;
    // the palette: transparent at 0, opaque at 15, never less opaque on the way
    auto alpha = [&](int i) { return int(t.palette[i] >> 24); };
    if (alpha(0) > 0x30 || alpha(15) < 0xB0) return false;
    for (int i = 1; i < 16; i++)
        if (alpha(i) + 8 < alpha(i - 1)) return false;
    // the indices: coverage levels only, mostly empty, and much of the ink full (a letter, not a soft glow)
    size_t zero = 0, full = 0, ink = 0;
    for (size_t i = 0; i < size_t(w) * h; i++) {
        uint8_t v = t.data[i];
        if (v > 15) return false;
        zero += v == 0, full += v == 15, ink += v != 0;
    }
    if (zero * 10 < size_t(w) * h * 3 || !ink || full * 100 < ink * 30) return false;

    // the atlas in rows (the texture is swizzled), and its crisp copy, made once per atlas whatever its colours
    std::vector<uint8_t> cov(size_t(w) * h);
    uint32_t mx, my;
    Masks(w, h, mx, my);
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) cov[size_t(y) * w + x] = t.data[Deposit(x, mx) | Deposit(y, my)];
    uint64_t key = Hash(cov.data(), cov.size()) ^ (uint64_t(w) << 48 | uint64_t(h) << 32);
    std::vector<float> crisp;
    {
        std::lock_guard<std::mutex> g(g_lock);
        auto it = g_crisp.find(key);
        if (it != g_crisp.end()) crisp = it->second;
    }
    if (crisp.empty()) {
        crisp = Crisp(cov.data(), w, h, kScale);
        std::lock_guard<std::mutex> g(g_lock);
        if (g_crisp.size() >= 16) g_crisp.clear();
        g_crisp[key] = crisp;
        LOGI("ps5 text: a %ux%u font made sharp (%zu%% of its ink full)", w, h, ink ? full * 100 / ink : 0);
    }
    // its colours by coverage level, blended between levels, as rotk_fonts.cpp does
    const uint32_t W = w * kScale, H = h * kScale;
    out = {};
    out.width = W;
    out.height = H;
    out.format = gpu::pack::Format::BGRA8;  // D3DCOLOR as stored
    std::vector<uint8_t>& px = out.levels.emplace_back(size_t(W) * H * 4);
    for (size_t i = 0; i < size_t(W) * H; i++) {
        float c = std::clamp(crisp[i], 0.0f, 15.0f);
        uint32_t lo = uint32_t(c), hi = std::min(lo + 1, 15u);
        float a = c - lo;
        for (int k = 0; k < 4; k++) {
            float v = float((t.palette[lo] >> (8 * k)) & 255) * (1 - a) + float((t.palette[hi] >> (8 * k)) & 255) * a;
            px[i * 4 + k] = uint8_t(v + 0.5f);
        }
    }
    gpu::pack::MakeLevels(out);
    return true;
}

}  // namespace rotk::ps5art
