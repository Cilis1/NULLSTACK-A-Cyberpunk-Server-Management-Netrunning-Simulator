#include "core/simulation.hpp"
#include "core/replay_log.hpp"
#include "save/sqlite_save.hpp"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string_view>

int main(int argc, char **argv) {
    std::uint64_t seed = 1, ticks = 1000;
    std::filesystem::path loadPath, savePath, replayPath, recordPath;
    bool seedOrTicksSpecified = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--help") {
            std::cout << "Usage: nullstack_headless [--seed N] [--ticks N] [--load FILE] [--save FILE] [--replay FILE] [--record FILE]\n"
                         "--ticks advances from a loaded state; --seed is ignored when loading.\n"
                         "--replay supplies seed, ticks and commands; --record writes the executed log.\n";
            return 0;
        }
        if ((option != "--seed" && option != "--ticks" && option != "--load" && option != "--save" && option != "--replay" && option != "--record") || i + 1 >= argc) {
            std::cerr << "Unknown or incomplete option: " << option << '\n';
            return 2;
        }
        const std::string_view input(argv[++i]);
        if (option == "--load" || option == "--save" || option == "--replay" || option == "--record") {
            if (input.empty()) {
                std::cerr << "Expected a path for " << option << '\n';
                return 2;
            }
            auto &path = option == "--load" ? loadPath : option == "--save" ? savePath :
                         option == "--replay" ? replayPath : recordPath;
            path = std::filesystem::u8path(input);
            continue;
        }
        seedOrTicksSpecified = true;
        auto &target = option == "--seed" ? seed : ticks;
        const auto parsed = std::from_chars(input.data(), input.data() + input.size(), target);
        if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size()) {
            std::cerr << "Expected an unsigned integer for " << option << '\n';
            return 2;
        }
    }
    if ((!replayPath.empty() && (seedOrTicksSpecified || !loadPath.empty())) ||
        (!recordPath.empty() && !loadPath.empty())) {
        std::cerr << "Replay cannot be combined with seed, ticks or load; record cannot be combined with load\n";
        return 2;
    }
    ns::ReplayLog log{seed, ticks, {}};
    if (!replayPath.empty()) {
        std::ifstream input(replayPath, std::ios::binary);
        const auto parsed = ns::readReplay(input, log);
        if (!parsed.ok) {
            std::cerr << "Replay failed: " << parsed.message << '\n';
            return 1;
        }
    }
    ns::Simulation sim(log.seed);
    if (!loadPath.empty()) {
        const auto result = ns::save::load(loadPath, sim);
        if (!result.ok) {
            std::cerr << "Load failed: " << result.message << '\n';
            return 1;
        }
    }
    const auto result = ns::replay(sim, log.commands, log.ticks);
    if (!result.ok) {
        std::cerr << result.message << '\n';
        return 1;
    }
    if (!recordPath.empty()) {
        std::ofstream output(recordPath, std::ios::binary | std::ios::trunc);
        const auto recorded = ns::writeReplay(output, log);
        output.flush();
        if (!recorded.ok || !output) {
            std::cerr << "Replay recording failed\n";
            return 1;
        }
    }
    if (!savePath.empty()) {
        const auto saved = ns::save::write(savePath, sim);
        if (!saved.ok) {
            std::cerr << "Save failed: " << saved.message << '\n';
            return 1;
        }
    }
    std::cout << "tick=" << sim.tickCount() << " credits=" << sim.credits() << " hash="
              << sim.stateHash() << '\n';
}
