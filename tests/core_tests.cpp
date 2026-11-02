#include "core/simulation.hpp"
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << " check failed: " #x "\n"; return 1; } } while (false)
int main() {
    ns::Rng rng(0);
    CHECK(rng.next() == 0xe220a8397b1dcdafULL);
    CHECK(rng.next() == 0x6e789e6aa1b965f4ULL);
    CHECK(!ns::Id{}.valid());
    CHECK(ns::Id{1}.valid());
    ns::Simulation a(42), b(42), different(43);
    int events = 0;
    a.events().subscribe([&](const ns::Event &) { ++events; });
    const std::vector<ns::RecordedCommand> commands{{0, {ns::CommandType::AddCredits, 100}},
                                                     {5, {ns::CommandType::SpendCredits, 40}}};
    CHECK(ns::replay(a, commands, 100000).ok);
    CHECK(ns::replay(b, commands, 100000).ok);
    CHECK(ns::replay(different, commands, 100000).ok);
    CHECK(a.stateHash() == b.stateHash());
    CHECK(a.stateHash() != different.stateHash());
    CHECK(a.credits() == 60 && events == 100002);
    CHECK(!a.apply({ns::CommandType::SpendCredits, 61}).ok);
    CHECK(!a.apply({ns::CommandType::AddCredits, -1}).ok);
    CHECK(!a.apply({ns::CommandType::AddCredits, std::numeric_limits<std::int64_t>::max()}).ok);
    CHECK(!a.apply({static_cast<ns::CommandType>(99), 1}).ok);
    ns::Simulation golden(42);
    CHECK(ns::replay(golden, {}, 100000).ok);
    CHECK(golden.stateHash() == 8781621828440958973ULL);
    ns::Simulation c(1);
    CHECK(!ns::replay(c, {{2, {ns::CommandType::AddCredits, 1}}, {1, {ns::CommandType::AddCredits, 1}}}, 3).ok);
    CHECK(c.tickCount() == 0);
    CHECK(!ns::replay(c, {{3, {ns::CommandType::AddCredits, 1}}}, 3).ok);
    std::cout << "core tests passed\n";
}
