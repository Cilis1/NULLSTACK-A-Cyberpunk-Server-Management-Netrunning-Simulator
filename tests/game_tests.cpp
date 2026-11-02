#include "core/game.hpp"
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
    ns::Game g(42), other(42);
    CHECK(!g.act(ns::GameAction::Repair).ok && g.state().day == 0);
    CHECK(g.act(ns::GameAction::BuyServer).ok);
    for (int i = 0; i < 4; ++i) CHECK(g.act(ns::GameAction::Host).ok);
    CHECK(g.act(ns::GameAction::BuyServer).ok);
    CHECK(!g.act(ns::GameAction::Host).ok && g.state().day == 6);
    CHECK(g.act(ns::GameAction::UpgradePower).ok);
    CHECK(g.act(ns::GameAction::BuyCooling).ok);
    CHECK(g.act(ns::GameAction::Host).ok);
    CHECK(g.state().condition == 65);
    ns::Game raid(1);
    while (raid.state().outcome == ns::GameOutcome::Playing) CHECK(raid.act(ns::GameAction::Intrude).ok);
    CHECK(raid.state().outcome == ns::GameOutcome::Raided || raid.state().outcome == ns::GameOutcome::Bankrupt);
    CHECK(!raid.act(ns::GameAction::LayLow).ok);
    ns::Game win(42);
    for (int i = 0; i < 150 && win.state().outcome == ns::GameOutcome::Playing; ++i) {
        const auto state = win.state();
        if (state.heat > 70) CHECK(win.act(ns::GameAction::LayLow).ok);
        else if (state.condition < 75) CHECK(win.act(ns::GameAction::Repair).ok);
        else if (state.credits > 1000 && state.servers < 4 && state.power <= state.servers)
            CHECK(win.act(ns::GameAction::UpgradePower).ok);
        else if (state.credits > 1000 && state.servers < 4) CHECK(win.act(ns::GameAction::BuyServer).ok);
        else CHECK(win.act(ns::GameAction::Host).ok);
    }
    CHECK(win.state().outcome == ns::GameOutcome::Won);
    CHECK(other.act(ns::GameAction::Intrude).ok);
    ns::Game same(42);
    CHECK(same.act(ns::GameAction::Intrude).ok);
    CHECK(other.state().credits == same.state().credits && other.state().heat == same.state().heat);
    std::cout << "game tests passed\n";
}
