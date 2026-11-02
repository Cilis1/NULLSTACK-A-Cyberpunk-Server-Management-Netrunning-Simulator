#include "core/game.hpp"
#include "core/thermal.hpp"
#include <algorithm>
#include <limits>

namespace ns {
GameResult Game::restore(const GameSnapshot &snapshot) {
    const auto &s = snapshot.state;
    if (s.day < 0 || s.day > std::numeric_limits<int>::max() - 1 ||
        s.credits < -200 || s.credits > std::numeric_limits<int>::max() - 1000 ||
        s.servers < 1 || s.servers > 8 || s.cooling < 1 || s.cooling > 8 ||
        s.power < 2 || s.power > 8 || s.condition < 0 || s.condition > 100 ||
        s.heat < 0 || s.heat > 127 || s.temperature < 24 || s.temperature > 90 || s.reputation < 0 || s.reputation > s.day * 2LL)
        return {false, "Invalid save state."};
    switch (s.outcome) {
    case GameOutcome::Playing:
        if (s.credits < 0 || s.heat >= 100 || (s.credits >= 2500 && s.servers >= 4 && s.reputation >= 5))
            return {false, "Inconsistent save outcome."};
        break;
    case GameOutcome::Won:
        if (s.heat >= 100 || s.credits < 2500 || s.servers < 4 || s.reputation < 5)
            return {false, "Inconsistent save outcome."};
        break;
    case GameOutcome::Bankrupt:
        if (s.credits >= 0 || s.heat >= 100) return {false, "Inconsistent save outcome."};
        break;
    case GameOutcome::Raided:
        if (s.heat < 100) return {false, "Inconsistent save outcome."};
        break;
    default: return {false, "Invalid save outcome."};
    }
    m_state = s;
    m_rng = Rng(snapshot.rngState);
    return {true, "ok"};
}
GameResult Game::act(GameAction action) {
    if (m_state.outcome != GameOutcome::Playing) return {false, "The run is over."};
    int price = 0;
    switch (action) {
    case GameAction::BuyServer: price = 300; break;
    case GameAction::BuyCooling: price = 180; break;
    case GameAction::UpgradePower: price = 220; break;
    case GameAction::Repair: price = 100; break;
    case GameAction::Host:
    case GameAction::Intrude:
    case GameAction::LayLow:
    case GameAction::Archive: break;
    default: return {false, "Unknown action."};
    }
    if (m_state.credits < price) return {false, "Not enough credits."};
    if (action == GameAction::BuyServer && m_state.servers >= 8) return {false, "The room holds at most eight servers."};
    if (action == GameAction::BuyCooling && m_state.cooling >= 8) return {false, "Cooling is already at its limit."};
    if (action == GameAction::UpgradePower && m_state.power >= 8) return {false, "The circuit is already at its limit."};
    if (action == GameAction::Repair && m_state.condition == 100) return {false, "Nothing needs repair."};
    if ((action == GameAction::Host || action == GameAction::Archive) && m_state.servers > m_state.power)
        return {false, "Circuit overloaded. Upgrade power before running workloads."};
    if (action == GameAction::Archive && (m_state.servers < 2 || m_state.reputation < 2))
        return {false, "Archive work requires two servers and two reputation."};
    if (action == GameAction::Archive && m_state.condition < 50)
        return {false, "Archive clients require at least 50% condition."};
    m_state.credits -= price;
    m_state.temperature = nextTemperature(m_state.temperature, m_state.servers, m_state.cooling,
                                          action == GameAction::Host || action == GameAction::Archive);
    std::string_view message;
    switch (action) {
    case GameAction::Host:
    case GameAction::Archive: {
        const bool archive = action == GameAction::Archive;
        const int capacity = archive ? 240 + m_state.servers * 60 : m_state.servers * 110;
        const int income = capacity * std::max(m_state.condition, 20) * thermalThroughput(m_state.temperature) / 10000;
        m_state.credits += income;
        m_state.reputation += archive ? 2 : 1;
        m_state.condition = std::max(0, m_state.condition - 2 - std::max(0, m_state.servers - m_state.cooling) * 5
                                         - std::max(0, m_state.temperature - 45) / 5);
        m_state.heat += archive ? 5 : 2;
        message = m_state.temperature > 45 ? "Thermal throttling reduced workload income and increased wear."
                 : archive ? "The Archivists pay for preserving a banned collection."
                           : "Hosting contracts paid out; equipment accumulated wear.";
        break;
    }
    case GameAction::Intrude:
        if (m_rng.next() % 100 < 65) {
            m_state.credits += 350;
            m_state.heat += 16;
            message = "Data extracted and sold. The logs are not as clean as you hoped.";
        } else {
            m_state.credits = std::max(0, m_state.credits - 100);
            m_state.heat += 28;
            m_state.condition = std::max(0, m_state.condition - 8);
            message = "ICE burned the connection. Recovery costs 100 credits.";
        }
        break;
    case GameAction::BuyServer: ++m_state.servers; message = "A used server joins the rack."; break;
    case GameAction::BuyCooling: ++m_state.cooling; message = "A louder fan bank keeps the rack cooler."; break;
    case GameAction::UpgradePower: ++m_state.power; message = "The electrician asks no questions."; break;
    case GameAction::Repair: m_state.condition = std::min(100, m_state.condition + 35); message = "Maintenance restores condition."; break;
    case GameAction::LayLow: m_state.heat = std::max(0, m_state.heat - 18); message = "You disappear into paperwork for a day."; break;
    }
    ++m_state.day;
    // Rent and power are charged daily even while idle. The physical layer affects profit.
    m_state.credits -= 25 + m_state.servers * 10 + m_state.cooling * 5;
    if (action == GameAction::Host || action == GameAction::Archive) m_state.credits -= m_state.servers * 8;
    if (m_state.heat >= 100) m_state.outcome = GameOutcome::Raided;
    else if (m_state.credits < 0) m_state.outcome = GameOutcome::Bankrupt;
    else if (m_state.credits >= 2500 && m_state.servers >= 4 && m_state.reputation >= 5)
        m_state.outcome = GameOutcome::Won;
    return {true, message};
}
} // namespace ns
