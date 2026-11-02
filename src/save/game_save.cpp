#include "save/game_save.hpp"
#include <sqlite3.h>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>

namespace ns::save {
namespace {
using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
constexpr int APP_ID = 0x4e534247; // NSBG: distinct from the headless prototype's schema.
bool exec(sqlite3 *db, const char *sql) { return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK; }
Statement prepare(sqlite3 *db, const char *sql) {
    sqlite3_stmt *raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) raw = nullptr;
    return Statement(raw, sqlite3_finalize);
}
Database open(const std::filesystem::path &path, int flags) {
    sqlite3 *raw = nullptr;
    const auto name = path.u8string();
    sqlite3_open_v2(reinterpret_cast<const char *>(name.c_str()), &raw, flags, nullptr);
    return Database(raw, sqlite3_close);
}
int pragma(sqlite3 *db, const char *sql) {
    auto stmt = prepare(db, sql);
    return stmt && sqlite3_step(stmt.get()) == SQLITE_ROW ? sqlite3_column_int(stmt.get(), 0) : -1;
}
SaveResult failure(sqlite3 *db, const char *context) {
    return {false, std::string(context) + ": " + (db ? sqlite3_errmsg(db) : "database allocation failed")};
}
std::array<unsigned char, 8> encode(std::uint64_t n) {
    std::array<unsigned char, 8> bytes{};
    for (unsigned i = 0; i < 8; ++i) bytes[i] = static_cast<unsigned char>(n >> (i * 8U));
    return bytes;
}
std::uint64_t decode(const unsigned char *bytes) {
    std::uint64_t n = 0;
    for (unsigned i = 0; i < 8; ++i) n |= static_cast<std::uint64_t>(bytes[i]) << (i * 8U);
    return n;
}
} // namespace

SaveResult writeGame(const std::filesystem::path &path, const Game &game) {
    auto db = open(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    if (!db || sqlite3_errcode(db.get()) != SQLITE_OK) return failure(db.get(), "open save");
    if (!exec(db.get(), "BEGIN IMMEDIATE")) return failure(db.get(), "begin save");
    const auto rollback = [&](const char *message) {
        auto result = failure(db.get(), message); exec(db.get(), "ROLLBACK"); return result;
    };
    const int app = pragma(db.get(), "PRAGMA application_id");
    const int ver = pragma(db.get(), "PRAGMA user_version");
    if (app == 0 && ver == 0) {
        auto count = prepare(db.get(), "SELECT count(*) FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'");
        if (!count || sqlite3_step(count.get()) != SQLITE_ROW || sqlite3_column_int(count.get(), 0) != 0) {
            exec(db.get(), "ROLLBACK"); return {false, "Not an empty Back Room save database."};
        }
        count.reset();
        if (!exec(db.get(), "CREATE TABLE game (id INTEGER PRIMARY KEY CHECK(id=1), day INTEGER NOT NULL, credits INTEGER NOT NULL, servers INTEGER NOT NULL, cooling INTEGER NOT NULL, power INTEGER NOT NULL, condition INTEGER NOT NULL, heat INTEGER NOT NULL, reputation INTEGER NOT NULL, outcome INTEGER NOT NULL, rng BLOB NOT NULL, temperature INTEGER NOT NULL); PRAGMA application_id=1314079303; PRAGMA user_version=2"))
            return rollback("create save schema");
    } else if (app == APP_ID && ver == 1) {
        if (!exec(db.get(), "ALTER TABLE game ADD COLUMN temperature INTEGER NOT NULL DEFAULT 24; PRAGMA user_version=2"))
            return rollback("migrate save schema");
    } else if (app != APP_ID || ver != 2) {
        exec(db.get(), "ROLLBACK"); return {false, "Unsupported or unrelated save database."};
    }
    auto stmt = prepare(db.get(), "INSERT OR REPLACE INTO game (id,day,credits,servers,cooling,power,condition,heat,reputation,outcome,rng,temperature) VALUES(1,?,?,?,?,?,?,?,?,?,?,?)");
    if (!stmt) return rollback("prepare save");
    const auto snapshot = game.snapshot();
    const auto &s = snapshot.state;
    const std::array<int, 9> values{s.day, s.credits, s.servers, s.cooling, s.power, s.condition, s.heat,
                                    s.reputation, static_cast<int>(s.outcome)};
    for (std::size_t i = 0; i < values.size(); ++i)
        if (sqlite3_bind_int(stmt.get(), static_cast<int>(i + 1), values[i]) != SQLITE_OK) return rollback("bind save");
    const auto rng = encode(snapshot.rngState);
    if (sqlite3_bind_blob(stmt.get(), 10, rng.data(), 8, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_int(stmt.get(), 11, s.temperature) != SQLITE_OK ||
        sqlite3_step(stmt.get()) != SQLITE_DONE) return rollback("write save");
    stmt.reset();
    if (!exec(db.get(), "COMMIT")) return rollback("commit save");
    return {true, "ok"};
}
SaveResult loadGame(const std::filesystem::path &path, Game &game) {
    auto db = open(path, SQLITE_OPEN_READONLY);
    if (!db || sqlite3_errcode(db.get()) != SQLITE_OK) return failure(db.get(), "open save");
    if (!exec(db.get(), "BEGIN")) return failure(db.get(), "begin load");
    const int version = pragma(db.get(), "PRAGMA user_version");
    if (pragma(db.get(), "PRAGMA application_id") != APP_ID || (version != 1 && version != 2))
        return {false, "Unsupported or unrelated save database."};
    auto stmt = prepare(db.get(), version == 1
        ? "SELECT day,credits,servers,cooling,power,condition,heat,reputation,outcome,rng,24 FROM game WHERE id=1"
        : "SELECT day,credits,servers,cooling,power,condition,heat,reputation,outcome,rng,temperature FROM game WHERE id=1");
    if (!stmt || sqlite3_step(stmt.get()) != SQLITE_ROW) return {false, "Missing or unreadable run."};
    std::array<int, 9> v{};
    for (int i = 0; i < 9; ++i) {
        if (sqlite3_column_type(stmt.get(), i) != SQLITE_INTEGER) return {false, "Invalid save fields."};
        const auto value = sqlite3_column_int64(stmt.get(), i);
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
            return {false, "Save field out of range."};
        v[static_cast<std::size_t>(i)] = static_cast<int>(value);
    }
    if (sqlite3_column_type(stmt.get(), 9) != SQLITE_BLOB || sqlite3_column_bytes(stmt.get(), 9) != 8)
        return {false, "Invalid random state."};
    if (sqlite3_column_type(stmt.get(), 10) != SQLITE_INTEGER ||
        sqlite3_column_int64(stmt.get(), 10) < 24 || sqlite3_column_int64(stmt.get(), 10) > 90)
        return {false, "Invalid rack temperature."};
    const int temperature = sqlite3_column_int(stmt.get(), 10);
    GameSnapshot snapshot{{v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], static_cast<GameOutcome>(v[8]), temperature},
                          decode(static_cast<const unsigned char *>(sqlite3_column_blob(stmt.get(), 9)))};
    stmt.reset();
    if (!exec(db.get(), "COMMIT")) return failure(db.get(), "commit load");
    const auto result = game.restore(snapshot);
    return {result.ok, std::string(result.message)};
}
} // namespace ns::save
