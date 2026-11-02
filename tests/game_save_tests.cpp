#include "save/game_save.hpp"
#include <sqlite3.h>
#include <filesystem>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; return 1; } } while (false)
int main() {
    const std::filesystem::path path = "back-room-save-test.sqlite";
    std::filesystem::remove(path);
    ns::Game original(42), restored(999);
    CHECK(original.act(ns::GameAction::Host).ok);
    CHECK(original.act(ns::GameAction::Intrude).ok);
    CHECK(ns::save::writeGame(path, original).ok);
    CHECK(ns::save::loadGame(path, restored).ok);
    CHECK(restored.state().day == original.state().day);
    CHECK(restored.state().credits == original.state().credits);
    CHECK(restored.state().temperature == original.state().temperature);
    CHECK(restored.snapshot().rngState == original.snapshot().rngState);
    // Continuing both runs yields the same random outcome, not merely the same visible state.
    CHECK(original.act(ns::GameAction::Intrude).ok);
    CHECK(restored.act(ns::GameAction::Intrude).ok);
    CHECK(original.state().heat == restored.state().heat);
    CHECK(original.state().credits == restored.state().credits);
    CHECK(ns::save::writeGame(path, original).ok);
    CHECK(ns::save::loadGame(path, restored).ok);
    sqlite3 *db = nullptr;
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "UPDATE game SET servers=900", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    const auto before = restored.snapshot();
    CHECK(!ns::save::loadGame(path, restored).ok);
    CHECK(restored.state().day == before.state.day && restored.snapshot().rngState == before.rngState);
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "PRAGMA user_version=99", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    CHECK(!ns::save::writeGame(path, original).ok);
    CHECK(!ns::save::loadGame(path, restored).ok);
    std::filesystem::remove(path);
    // A v1 slot loads with ambient temperature, then migrates atomically on write.
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "CREATE TABLE game(id INTEGER PRIMARY KEY,day INTEGER,credits INTEGER,servers INTEGER,cooling INTEGER,power INTEGER,condition INTEGER,heat INTEGER,reputation INTEGER,outcome INTEGER,rng BLOB); INSERT INTO game VALUES(1,1,800,1,1,2,98,2,1,0,x'0000000000000000'); PRAGMA application_id=1314079303; PRAGMA user_version=1", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    CHECK(ns::save::loadGame(path, restored).ok);
    CHECK(restored.state().temperature == 24 && restored.state().day == 1);
    CHECK(restored.act(ns::GameAction::Host).ok && restored.state().temperature == 27);
    CHECK(ns::save::writeGame(path, restored).ok);
    CHECK(ns::save::loadGame(path, original).ok && original.state().temperature == 27);
    std::filesystem::remove(path);
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "CREATE TABLE other(x)", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    CHECK(!ns::save::writeGame(path, original).ok);
    std::filesystem::remove(path);
    CHECK(!ns::save::loadGame(path, restored).ok);
    std::cout << "game save tests passed\n";
}
