#include "core/replay_log.hpp"
#include <iostream>
#include <sstream>
#include <string>

#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << " check failed: " #x "\n"; return 1; } } while (false)
int main() {
    ns::ReplayLog log{42, 100000, {{0, {ns::CommandType::AddCredits, 100}}, {5, {ns::CommandType::SpendCredits, 40}}}};
    std::ostringstream encoded;
    CHECK(ns::writeReplay(encoded, log).ok);
    ns::ReplayLog decoded;
    std::istringstream input(encoded.str());
    CHECK(ns::readReplay(input, decoded).ok);
    ns::Simulation first(log.seed), second(decoded.seed);
    CHECK(ns::replay(first, log.commands, log.ticks).ok);
    CHECK(ns::replay(second, decoded.commands, decoded.ticks).ok);
    CHECK(first.stateHash() == second.stateHash() && second.credits() == 60);
    for (const std::string &bad : {"NULLSTACK_REPLAY 2\nseed 1\nticks 2\n", "NULLSTACK_REPLAY 1\nseed -1\nticks 2\n", "NULLSTACK_REPLAY 1\nseed 1\nticks 2\ncommand 2 add 1\n", "NULLSTACK_REPLAY 1\nseed 1\nticks 2\ncommand 0 add 1 extra\n", "NULLSTACK_REPLAY 1\nseed 1\nticks 2\ncommand 0 nope 1\n"}) {
        std::istringstream malformed(bad);
        CHECK(!ns::readReplay(malformed, decoded).ok);
        CHECK(decoded.seed == 42 && decoded.commands.size() == 2);
    }
    std::istringstream longLine("NULLSTACK_REPLAY 1\nseed 1\nticks 1\n" + std::string(257, 'x'));
    CHECK(!ns::readReplay(longLine, decoded).ok);
    std::ostringstream invalid;
    log.commands[1].tick = log.ticks;
    CHECK(!ns::writeReplay(invalid, log).ok && invalid.str().empty());
    std::cout << "replay tests passed\n";
}
