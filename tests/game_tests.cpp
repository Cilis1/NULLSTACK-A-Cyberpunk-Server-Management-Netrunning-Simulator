#include "core/game.hpp"
#include "core/thermal.hpp"
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
    CHECK(ns::nextTemperature(24, 4, 1, true) == 29);
    CHECK(ns::nextTemperature(55, 4, 1, false) == 50);
    CHECK(ns::nextTemperature(24, 4, 8, true) == 24);
    CHECK(ns::thermalThroughput(45) == 100);
    CHECK(ns::thermalThroughput(50) == 75);
    CHECK(ns::thermalThroughput(80) == 50);
    ns::Game archive(1);
    CHECK(!archive.act(ns::GameAction::Archive).ok && archive.state().day == 0);
    CHECK(archive.act(ns::GameAction::BuyServer).ok);
    CHECK(archive.act(ns::GameAction::Host).ok);
    CHECK(archive.act(ns::GameAction::Host).ok);
    const auto beforeArchive = archive.state();
    CHECK(archive.act(ns::GameAction::Archive).ok);
    CHECK(archive.state().reputation == beforeArchive.reputation + 2);
    CHECK(archive.state().heat == beforeArchive.heat + 5);
    CHECK(archive.state().credits > beforeArchive.credits);
    ns::Game copy(1);
    CHECK(copy.restore(archive.snapshot()).ok);
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
    // Overheating persists across days, throttles production, then cools while idle.
    ns::Game thermal(1);
    auto hot = thermal.snapshot();
    hot.state.servers = 4; hot.state.power = 4; hot.state.credits = 1000;
    CHECK(thermal.restore(hot).ok);
    for (int i = 0; i < 5; ++i) CHECK(thermal.act(ns::GameAction::Host).ok);
    CHECK(thermal.state().temperature > 45);
    const int hotTemperature = thermal.state().temperature;
    CHECK(thermal.act(ns::GameAction::LayLow).ok);
    CHECK(thermal.state().temperature < hotTemperature);
    ns::Game hotRun(1), coolRun(1);
    auto hotState = hotRun.snapshot();
    hotState.state.servers = 4; hotState.state.power = 4; hotState.state.credits = 1000;
    hotState.state.temperature = 60;
    auto coolState = hotState; coolState.state.temperature = 24;
    CHECK(hotRun.restore(hotState).ok && coolRun.restore(coolState).ok);
    CHECK(hotRun.act(ns::GameAction::Host).ok && coolRun.act(ns::GameAction::Host).ok);
    CHECK(hotRun.state().credits < coolRun.state().credits);
    CHECK(hotRun.state().condition < coolRun.state().condition);
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
