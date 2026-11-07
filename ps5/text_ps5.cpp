// rotk-ps5: the game's interface text where it names the Xbox or its controller's buttons, in its PS5 wording.
//
// Two functions give the game its lines: GetText 0xF6DC0 (key, language, flags), which the menus' widgets ask every
// frame (the port's rotk_lang.cpp hooks it and calls After once the game's own lookup has returned), and 0xF75F0
// (key, ...), which fills in a line's % fields into the buffer 0x1D9D10: the save and load messages ("Please don't
// turn off your Xbox console") come through that one. After either, a line that says START (the button, in
// capitals) or Xbox is given in its PS5 wording: OPTIONS, PS5. Button pictures are rotk_prompts.cpp's.
#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include "cpu/x86_context.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"

namespace rotk::ps5text {

namespace {

// interface line name (lower case) -> what it says on a PS5
const std::unordered_map<std::string, std::string> kText = {
    {"lefttriggerbtnname", "L2"},
    {"righttriggerbtnname", "R2"},
};

std::mutex g_lock;
std::unordered_map<std::string, uint32_t> g_made;    // our lines in guest memory, made once each
std::unordered_map<std::string, uint32_t> g_by_key;  // GetText: key -> our line in its place (0: the game's own)

std::string Read(uint32_t va, size_t max) {
    std::string s;
    for (size_t i = 0; va && i < max; i++) {
        char c = char(kernel::G<uint8_t>(va + uint32_t(i)));
        if (!c) break;
        s += c;
    }
    return s;
}

std::string Lower(std::string s) {
    for (char& c : s) c = char(std::tolower(uint8_t(c)));
    return s;
}

// a word on its own: no letter either side
bool Alone(const std::string& s, size_t at, size_t n) {
    return (at == 0 || !std::isalpha(uint8_t(s[at - 1]))) && (at + n >= s.size() || !std::isalpha(uint8_t(s[at + n])));
}

// START (the button, in capitals; "Start" in a sentence stays) as OPTIONS, and Xbox (with the trade mark sign the
// game's font draws after it: § or ®, Latin-1) as PS5
bool Ps5(std::string& s) {
    bool changed = false;
    for (size_t at = s.find("START"); at != std::string::npos; at = s.find("START", at + 1)) {
        if (!Alone(s, at, 5)) continue;
        s.replace(at, 5, "OPTIONS");
        changed = true;
    }
    for (const char* w : {"Xbox", "XBOX"}) {
        for (size_t at = s.find(w); at != std::string::npos; at = s.find(w, at + 1)) {
            if (!Alone(s, at, 4)) continue;
            size_t n = 4;
            if (at + 4 < s.size() && (uint8_t(s[at + 4]) == 0xA7 || uint8_t(s[at + 4]) == 0xAE)) n = 5;
            s.replace(at, n, "PS5");
            changed = true;
        }
    }
    return changed;
}

uint32_t Guest(const std::string& text) {
    auto it = g_made.find(text);
    if (it != g_made.end()) return it->second;
    if (g_made.size() >= 1024) return 0;  // a line made of changing numbers: never a store of them
    uint32_t va = mem::AllocSystem(uint32_t(text.size() + 16) & ~15u, 16);
    if (va) std::memcpy(kernel::HostPtr(va), text.c_str(), text.size() + 1);
    g_made[text] = va;
    return va;
}

}  // namespace

// after GetText(key, language, flags) returned its line in eax
void After(X86Context& ctx, uint32_t key_va) {
    if (!key_va || !ctx.eax) return;
    std::string key = Lower(Read(key_va, 96));
    std::lock_guard lock(g_lock);
    if (auto r = kText.find(key); r != kText.end()) {
        if (uint32_t ours = Guest(r->second)) ctx.eax = ours;
        return;
    }
    // a line seen before: ours in its place, or the game's own
    if (auto b = g_by_key.find(key); b != g_by_key.end()) {
        if (b->second) ctx.eax = b->second;
        return;
    }
    std::string ours = Read(ctx.eax, 1024);
    uint32_t va = Ps5(ours) ? Guest(ours) : 0;
    if (g_by_key.size() < 4096) g_by_key[key] = va;
    if (va) ctx.eax = va;
}

// after 0xF75F0 filled in a line (its % fields) into its buffer, returned in eax: a shorter wording is written in
// place, a longer one is our copy
void AfterFilled(X86Context& ctx) {
    if (!ctx.eax) return;
    std::string text = Read(ctx.eax, 1024), ours = text;
    if (!Ps5(ours)) return;
    if (ours.size() <= text.size()) {
        std::memcpy(kernel::HostPtr(ctx.eax), ours.c_str(), ours.size() + 1);
        return;
    }
    std::lock_guard lock(g_lock);
    if (uint32_t va = Guest(ours)) ctx.eax = va;
}

}  // namespace rotk::ps5text

X86_EXTERN_FUNC(__imp__sub_000F75F0);
X86_FUNC(sub_000F75F0) {
    __imp__sub_000F75F0(ctx, base);
    rotk::ps5text::AfterFilled(ctx);
}
