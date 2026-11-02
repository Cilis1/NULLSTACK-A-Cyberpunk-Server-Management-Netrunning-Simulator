#pragma once
#include "core/simulation.hpp"
#include <cstdint>
#include <string_view>

namespace ns {
/// A deliberately small, turn-based early-access scenario. All units are abstract game units.
enum class GameAction { Host, Intrude, BuyServer, BuyCooling, UpgradePower, Repair, LayLow, Archive };
enum class GameOutcome { Playing, Won, Bankrupt, Raided };
struct GameState {
    int day = 0;
    int credits = 800;
    int servers = 1;
    int cooling = 1;
    int power = 2;
    int condition = 100;
    int heat = 0;
    int reputation = 0;
    GameOutcome outcome = GameOutcome::Playing;
    int temperature = 24; // Aggregate rack temperature in degrees Celsius.
};
struct GameSnapshot {
    GameState state;
    std::uint64_t rngState;
};
struct GameResult { bool ok; std::string_view message; };
/// One accepted action advances one day. Rejected actions never change state or RNG.
class Game {
  public:
    explicit Game(std::uint64_t seed) : m_rng(seed) {}
    GameResult act(GameAction action);
    const GameState &state() const { return m_state; }
    /// Capture all state required to reproduce future actions.
    GameSnapshot snapshot() const { return {m_state, m_rng.state()}; }
    /// Reject malformed snapshots without changing the current run.
    GameResult restore(const GameSnapshot &snapshot);
  private:
    GameState m_state;
    Rng m_rng;
};
} // namespace ns
