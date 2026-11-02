#include "core/simulation.hpp"
#include "save/sqlite_save.hpp"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>

int main(int argc, char **argv) {
    std::uint64_t seed = 1, ticks = 1000;
    std::filesystem::path loadPath, savePath;
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--help") {
            std::cout << "Usage: nullstack_headless [--seed N] [--ticks N] [--load FILE] [--save FILE]\n"
                         "--ticks advances from the loaded state; --seed is ignored when loading.\n";
            return 0;
        }
        if ((option != "--seed" && option != "--ticks" && option != "--load" && option != "--save") || i + 1 >= argc) {
            std::cerr << "Unknown or incomplete option: " << option << '\n';
            return 2;
        }
        const std::string_view input(argv[++i]);
        if (option == "--load" || option == "--save") {
            if (input.empty()) {
                std::cerr << "Expected a path for " << option << '\n';
                return 2;
            }
            (option == "--load" ? loadPath : savePath) = std::filesystem::u8path(input);
            continue;
        }
        auto &target = option == "--seed" ? seed : ticks;
        const auto parsed = std::from_chars(input.data(), input.data() + input.size(), target);
        if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size()) {
            std::cerr << "Expected an unsigned integer for " << option << '\n';
            return 2;
        }
    }
    ns::Simulation sim(seed);
    if (!loadPath.empty()) {
        const auto result = ns::save::load(loadPath, sim);
        if (!result.ok) {
            std::cerr << "Load failed: " << result.message << '\n';
            return 1;
        }
    }
    const auto result = ns::replay(sim, {}, ticks);
    if (!result.ok) {
        std::cerr << result.message << '\n';
        return 1;
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
