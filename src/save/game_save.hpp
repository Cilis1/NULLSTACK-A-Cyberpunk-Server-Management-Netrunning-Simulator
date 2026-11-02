#pragma once
#include "core/game.hpp"
#include "save/sqlite_save.hpp"
#include <filesystem>

namespace ns::save {
/// Store one playable Back Room run transactionally; never overwrites another database type.
SaveResult writeGame(const std::filesystem::path &path, const Game &game);
/// Validate a saved run before replacing any live game state.
SaveResult loadGame(const std::filesystem::path &path, Game &game);
} // namespace ns::save
