// rotk-ps5: what the PS5 host's parts share
#pragma once
#include <filesystem>
#include <string>

namespace ps5 {

// /data/rotk: settings, saves, logs, caches, and where the player's disc image goes
const std::filesystem::path& DataDir();

// a system notification (the bubble in the corner), for errors there is no screen for yet
void Notify(const char* text);

// the player's copy of the game: a disc image (.iso) or extracted files with default.xbe
struct Game {
    std::filesystem::path path;  // the .iso, or the folder holding default.xbe
    bool image = false;
};
// /data/rotk/*.iso, /data/rotk/game, then /mnt/usb0..7/rotk/*.iso and /mnt/usbN/rotk/game; the first one found
bool FindGame(Game& game);

}  // namespace ps5
