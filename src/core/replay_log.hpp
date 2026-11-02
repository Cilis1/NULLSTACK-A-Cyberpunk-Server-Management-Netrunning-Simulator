#pragma once
#include "core/simulation.hpp"
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace ns {
/// Versioned, portable input log. Commands apply in recorded order before each tick.
struct ReplayLog {
    std::uint64_t seed{};
    std::uint64_t ticks{};
    std::vector<RecordedCommand> commands;
};
struct ReplayResult {
    bool ok;
    std::string message;
};
/// Read a strictly validated log; on error, output is unchanged. Up to 100,000 commands are accepted.
ReplayResult readReplay(std::istream &input, ReplayLog &output);
/// Validate and serialize a log using the version 1 text format.
ReplayResult writeReplay(std::ostream &output, const ReplayLog &log);
} // namespace ns
