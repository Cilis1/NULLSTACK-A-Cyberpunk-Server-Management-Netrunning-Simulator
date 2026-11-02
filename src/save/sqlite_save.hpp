#pragma once
#include "core/simulation.hpp"
#include <filesystem>
#include <string>

namespace ns::save {
/// Result owns its message so SQLite error strings never outlive their connection.
struct SaveResult {
    bool ok;
    std::string message;
};
/// Atomically replaces the snapshot in a version 1 SQLite database. Unknown schemas are rejected.
SaveResult write(const std::filesystem::path &path, const Simulation &simulation);
/// Loads a validated snapshot without modifying the simulation on any error.
SaveResult load(const std::filesystem::path &path, Simulation &simulation);
} // namespace ns::save
