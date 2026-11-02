#include "core/game.hpp"
#include <algorithm>

namespace ns {
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
    case GameAction::LayLow: break;
    default: return {false, "Unknown action."};
    }
    if (m_state.credits < price) return {false, "Not enough credits."};
    if (action == GameAction::BuyServer && m_state.servers >= 8) return {false, "The room holds at most eight servers."};
    if (action == GameAction::BuyCooling && m_state.cooling >= 8) return {false, "Cooling is already at its limit."};
    if (action == GameAction::UpgradePower && m_state.power >= 8) return {false, "The circuit is already at its limit."};
    if (action == GameAction::Repair && m_state.condition == 100) return {false, "Nothing needs repair."};
    if (action == GameAction::Host && m_state.servers > m_state.power) return {false, "Circuit overloaded. Upgrade power before hosting."};
    m_state.credits -= price;
    std::string_view message;
    switch (action) {
    case GameAction::Host: {
        const int capacity = m_state.servers * 110;
        const int income = capacity * std::max(m_state.condition, 20) / 100;
        m_state.credits += income;
        ++m_state.reputation;
        m_state.condition = std::max(0, m_state.condition - 2 - std::max(0, m_state.servers - m_state.cooling) * 5);
        m_state.heat += 2;
        message = "Hosting contracts paid out; equipment accumulated wear.";
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
    if (m_state.heat >= 100) m_state.outcome = GameOutcome::Raided;
    else if (m_state.credits < 0) m_state.outcome = GameOutcome::Bankrupt;
    else if (m_state.credits >= 2500 && m_state.servers >= 4 && m_state.reputation >= 5)
        m_state.outcome = GameOutcome::Won;
    return {true, message};
}
} // namespace ns
