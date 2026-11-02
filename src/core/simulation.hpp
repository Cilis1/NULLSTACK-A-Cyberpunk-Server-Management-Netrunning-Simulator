#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace ns {

/// Fixed-width identity. Zero is reserved as an invalid ID.
struct Id {
    std::uint64_t value{};
    constexpr bool valid() const { return value != 0; }
    bool operator==(const Id &) const = default;
};

/// A non-throwing result for player commands.
struct Result {
    bool ok;
    std::string_view message;
};

/// SplitMix64 with fully specified unsigned arithmetic, independent of standard distributions.
class Rng {
  public:
    explicit Rng(std::uint64_t seed) : m_state(seed) {}
    std::uint64_t next();
    std::uint64_t state() const { return m_state; }

  private:
    std::uint64_t m_state;
};

/// Events are delivered synchronously in subscription order. Subscribers must not mutate the bus.
struct Event {
    enum class Type { CommandApplied, TickAdvanced } type;
    std::uint64_t tick;
};
class EventBus {
  public:
    using Handler = std::function<void(const Event &)>;
    void subscribe(Handler handler);
    void publish(const Event &event) const;

  private:
    std::vector<Handler> m_handlers;
};

/// A tiny command vocabulary used to exercise the architecture, not final game content.
enum class CommandType : std::uint8_t { AddCredits, SpendCredits };
struct Command {
    CommandType type;
    std::int64_t amount;
};
struct RecordedCommand {
    std::uint64_t tick;
    Command command;
};

/// Deterministic headless simulation. Commands apply before advancing their specified tick.
class Simulation {
  public:
    explicit Simulation(std::uint64_t seed);
    Result apply(Command command);
    void tick();
    std::uint64_t tickCount() const { return m_tick; }
    std::int64_t credits() const { return m_credits; }
    std::uint64_t stateHash() const;
    EventBus &events() { return m_events; }

  private:
    std::uint64_t m_tick{};
    std::int64_t m_credits{};
    Rng m_rng;
    EventBus m_events;
};

/// Execute ordered commands at ticks [0, ticks); rejects out-of-order and invalid commands.
Result replay(Simulation &simulation, const std::vector<RecordedCommand> &commands,
              std::uint64_t ticks);
} // namespace ns
