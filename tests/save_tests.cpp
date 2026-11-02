#include "save/sqlite_save.hpp"
#include <sqlite3.h>
#include <filesystem>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << " check failed: " #x "\n"; return 1; } } while (false)
int main() {
    const std::filesystem::path path = "nullstack_save_test.sqlite";
    std::filesystem::remove(path);
    ns::Simulation original(123), restored(987);
    CHECK(original.apply({ns::CommandType::AddCredits, 500}).ok);
    CHECK(ns::replay(original, {{5, {ns::CommandType::SpendCredits, 75}}}, 100).ok);
    CHECK(ns::save::write(path, original).ok);
    CHECK(ns::save::load(path, restored).ok);
    CHECK(original.stateHash() == restored.stateHash());
    CHECK(ns::replay(original, {}, 1000).ok && ns::replay(restored, {}, 1000).ok);
    CHECK(original.stateHash() == restored.stateHash());
    CHECK(ns::save::write(path, original).ok);
    CHECK(ns::save::load(path, restored).ok && original.stateHash() == restored.stateHash());
    sqlite3 *db = nullptr;
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "UPDATE snapshot SET credits=-1", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    auto previous = restored.stateHash();
    CHECK(!ns::save::load(path, restored).ok);
    CHECK(previous == restored.stateHash());
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "UPDATE snapshot SET credits=1, tick=x'01'", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    CHECK(!ns::save::load(path, restored).ok && restored.stateHash() == previous);
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "PRAGMA user_version=99", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    CHECK(!ns::save::load(path, restored).ok && restored.stateHash() == previous);
    CHECK(!ns::save::write(path, original).ok);
    std::filesystem::remove(path);
    CHECK(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "CREATE TABLE unrelated (value INTEGER)", nullptr, nullptr, nullptr) == SQLITE_OK);
    CHECK(sqlite3_close(db) == SQLITE_OK);
    CHECK(!ns::save::write(path, original).ok);
    std::filesystem::remove(path);
    CHECK(!ns::save::load(path, restored).ok && restored.stateHash() == previous);
    std::cout << "save tests passed\n";
}
