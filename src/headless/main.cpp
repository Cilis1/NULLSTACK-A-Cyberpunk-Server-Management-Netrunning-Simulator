#include "core/simulation.hpp"
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string_view>

int main(int argc, char **argv) {
    std::uint64_t seed = 1, ticks = 1000;
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--help") {
            std::cout << "Usage: nullstack_headless [--seed N] [--ticks N]\n";
            return 0;
        }
        if ((option != "--seed" && option != "--ticks") || i + 1 >= argc) {
            std::cerr << "Unknown or incomplete option: " << option << '\n';
            return 2;
        }
        const std::string_view input(argv[++i]);
        auto &target = option == "--seed" ? seed : ticks;
        const auto parsed = std::from_chars(input.data(), input.data() + input.size(), target);
        if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size()) {
            std::cerr << "Expected an unsigned integer for " << option << '\n';
            return 2;
        }
    }
    ns::Simulation sim(seed);
    const auto result = ns::replay(sim, {}, ticks);
    if (!result.ok) {
        std::cerr << result.message << '\n';
        return 1;
    }
    std::cout << "tick=" << sim.tickCount() << " credits=" << sim.credits() << " hash="
              << sim.stateHash() << '\n';
}
