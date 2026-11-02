#include "core/game.hpp"
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {
void status(const ns::GameState &s) {
    std::cout << "\nDAY " << s.day << " | credits " << s.credits << " | reputation " << s.reputation
              << " | heat " << s.heat << "/100\nServers " << s.servers << "/8 | power " << s.power
              << " | cooling " << s.cooling << " | condition " << s.condition << "%\n"
              << "Daily overhead: " << 25 + s.servers * 10 + s.cooling * 5 << " credits\n";
}
void help() {
    std::cout << "\nCommands (one successful action = one day):\n"
              << "  host     Serve clients: 110 per server scaled by condition; +1 reputation, +2 heat.\n"
              << "  run      Intrude: 65% chance of 350 credits and 16 heat; failure costs 100 and 28 heat.\n"
              << "  server   Buy a server (300). Each server adds 10/day overhead. Max 8.\n"
              << "  cooling  Buy cooling (180); each adds 5/day overhead. Excess servers wear faster.\n"
              << "  power    Upgrade circuit (220); hosting requires power >= servers.\n"
              << "  repair   Restore 35 condition (100; maximum 100).\n"
              << "  laylow   Reduce heat by 18. Daily overhead still applies.\n"
              << "  status   Show the rack.  help   Show this text.  quit   Exit the run.\n"
              << "Win: hold 2500 credits, 4 servers and 5 reputation. Heat 100 causes a raid;\n"
              << "negative credits cause bankruptcy. Every action incurs rent and power.\n";
}
}
int main(int argc, char **argv) {
    std::uint64_t seed = 42;
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "Usage: Nullstack [unsigned-seed]\n"; return 0;
    }
    if (argc > 2) { std::cerr << "Usage: Nullstack [unsigned-seed]\n"; return 2; }
    if (argc == 2) {
        std::string_view input(argv[1]);
        const auto r = std::from_chars(input.data(), input.data() + input.size(), seed);
        if (r.ec != std::errc{} || r.ptr != input.data() + input.size()) { std::cerr << "Invalid seed.\n"; return 2; }
    }
    ns::Game game(seed);
    std::cout << "NULLSTACK // BACK ROOM\nA noodle shop. A rack. A very patient landlord.\n"
              << "Early-access terminal scenario. Seed " << seed << ". Type help for instructions.\n";
    help();
    while (game.state().outcome == ns::GameOutcome::Playing) {
        status(game.state());
        std::cout << "voidshell> " << std::flush;
        std::string input;
        if (!std::getline(std::cin, input) || input == "quit") break;
        if (input == "help") { help(); continue; }
        if (input == "status" || input.empty()) continue;
        ns::GameAction action;
        if (input == "host") action = ns::GameAction::Host;
        else if (input == "run") action = ns::GameAction::Intrude;
        else if (input == "server") action = ns::GameAction::BuyServer;
        else if (input == "cooling") action = ns::GameAction::BuyCooling;
        else if (input == "power") action = ns::GameAction::UpgradePower;
        else if (input == "repair") action = ns::GameAction::Repair;
        else if (input == "laylow") action = ns::GameAction::LayLow;
        else { std::cout << "Unknown command. Type help.\n"; continue; }
        const auto result = game.act(action);
        std::cout << result.message << '\n';
    }
    status(game.state());
    switch (game.state().outcome) {
    case ns::GameOutcome::Won: std::cout << "VICTORY // Your back room is a real operation.\n"; break;
    case ns::GameOutcome::Bankrupt: std::cout << "BANKRUPT // The landlord has changed the locks.\n"; break;
    case ns::GameOutcome::Raided: std::cout << "RAID // The door was cheaper than the warrant.\n"; break;
    case ns::GameOutcome::Playing: std::cout << "Disconnected. This scenario does not yet save progress.\n"; break;
    }
}
