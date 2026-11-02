#include "core/replay_log.hpp"
#include <charconv>
#include <istream>
#include <ostream>
#include <sstream>
#include <string_view>

namespace ns {
namespace {
constexpr std::size_t MAX_COMMANDS = 100000;
constexpr std::size_t MAX_LINE = 256;

template <typename T> bool number(std::string_view text, T &value) {
    if (text.empty()) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
ReplayResult validate(const ReplayLog &log) {
    if (log.commands.size() > MAX_COMMANDS) return {false, "too many commands"};
    std::uint64_t previous = 0;
    for (const auto &entry : log.commands) {
        if (entry.tick >= log.ticks || entry.tick < previous) return {false, "commands out of range or order"};
        if (entry.command.amount <= 0 || (entry.command.type != CommandType::AddCredits &&
                                           entry.command.type != CommandType::SpendCredits))
            return {false, "invalid command"};
        previous = entry.tick;
    }
    return {true, "ok"};
}
bool readLine(std::istream &input, std::string &line) {
    // Bound memory use even for malformed, untrusted replay files.
    line.clear();
    char c = 0;
    while (input.get(c)) {
        if (c == '\n') return true;
        if (line.size() == MAX_LINE) return false;
        line.push_back(c);
    }
    return !line.empty() && input.eof();
}
bool headerValue(const std::string &line, std::string_view prefix, std::uint64_t &value) {
    const std::string_view view(line);
    return view.starts_with(prefix) && number(view.substr(prefix.size()), value);
}
} // namespace

ReplayResult readReplay(std::istream &input, ReplayLog &output) {
    ReplayLog candidate;
    std::string line;
    if (!readLine(input, line) || line != "NULLSTACK_REPLAY 1") return {false, "invalid replay header"};
    if (!readLine(input, line) || !headerValue(line, "seed ", candidate.seed)) return {false, "invalid seed"};
    if (!readLine(input, line) || !headerValue(line, "ticks ", candidate.ticks)) return {false, "invalid tick count"};
    while (readLine(input, line)) {
        if (candidate.commands.size() == MAX_COMMANDS) return {false, "too many commands"};
        std::istringstream fields(line);
        std::string label, tick, type, amount, extra;
        if (!(fields >> label >> tick >> type >> amount) || fields >> extra || label != "command")
            return {false, "invalid command line"};
        RecordedCommand entry{};
        if (!number(tick, entry.tick) || !number(amount, entry.command.amount)) return {false, "invalid command number"};
        if (type == "add") entry.command.type = CommandType::AddCredits;
        else if (type == "spend") entry.command.type = CommandType::SpendCredits;
        else return {false, "unknown command type"};
        candidate.commands.push_back(entry);
    }
    if (!input.eof()) return {false, "replay line too long or input error"};
    const auto result = validate(candidate);
    if (!result.ok) return result;
    output = std::move(candidate);
    return {true, "ok"};
}
ReplayResult writeReplay(std::ostream &output, const ReplayLog &log) {
    const auto result = validate(log);
    if (!result.ok) return result;
    output << "NULLSTACK_REPLAY 1\nseed " << log.seed << "\nticks " << log.ticks << '\n';
    for (const auto &entry : log.commands) {
        output << "command " << entry.tick << ' '
               << (entry.command.type == CommandType::AddCredits ? "add" : "spend") << ' '
               << entry.command.amount << '\n';
    }
    return output ? ReplayResult{true, "ok"} : ReplayResult{false, "replay write failed"};
}
} // namespace ns
