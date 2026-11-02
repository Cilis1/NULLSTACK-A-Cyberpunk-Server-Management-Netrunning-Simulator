#include "core/simulation.hpp"

#include <limits>
#include <utility>

namespace ns {
std::uint64_t Rng::next() {
    m_state += 0x9e3779b97f4a7c15ULL;
    auto value = m_state;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}
void EventBus::subscribe(Handler handler) { m_handlers.push_back(std::move(handler)); }
void EventBus::publish(const Event &event) const {
    for (const auto &handler : m_handlers) {
        handler(event);
    }
}
Simulation::Simulation(std::uint64_t seed) : m_rng(seed) {}
Result Simulation::apply(Command command) {
    if (command.amount <= 0) {
        return {false, "amount must be positive"};
    }
    switch (command.type) {
    case CommandType::AddCredits:
        if (m_credits > std::numeric_limits<std::int64_t>::max() - command.amount) {
            return {false, "credit overflow"};
        }
        m_credits += command.amount;
        break;
    case CommandType::SpendCredits:
        if (command.amount > m_credits) {
            return {false, "insufficient credits"};
        }
        m_credits -= command.amount;
        break;
    default:
        return {false, "unknown command"};
    }
    m_events.publish({Event::Type::CommandApplied, m_tick});
    return {true, "ok"};
}
void Simulation::tick() {
    (void)m_rng.next();
    ++m_tick;
    m_events.publish({Event::Type::TickAdvanced, m_tick});
}
std::uint64_t Simulation::stateHash() const {
    // FNV-1a over explicit little-endian fixed-width fields; no padding or native byte order.
    std::uint64_t hash = 14695981039346656037ULL;
    for (auto value : {m_tick, static_cast<std::uint64_t>(m_credits), m_rng.state()}) {
        for (unsigned i = 0; i < 8; ++i) {
            hash ^= (value >> (i * 8U)) & 0xffU;
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}
Result replay(Simulation &simulation, const std::vector<RecordedCommand> &commands,
              std::uint64_t ticks) {
    if (ticks > std::numeric_limits<std::uint64_t>::max() - simulation.tickCount()) {
        return {false, "tick range overflow"};
    }
    const auto end = simulation.tickCount() + ticks;
    // Validate the entire schedule before applying it, including commands beyond the end.
    auto previous = simulation.tickCount();
    for (const auto &entry : commands) {
        if (entry.tick < previous || entry.tick >= end) {
            return {false, "command tick out of range or order"};
        }
        previous = entry.tick;
    }
    std::size_t index = 0;
    while (simulation.tickCount() < end) {
        while (index < commands.size() && commands[index].tick == simulation.tickCount()) {
            const auto result = simulation.apply(commands[index++].command);
            if (!result.ok) {
                return result;
            }
        }
        simulation.tick();
    }
    return {true, "ok"};
}
} // namespace ns
