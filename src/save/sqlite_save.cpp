#include "save/sqlite_save.hpp"
#include <sqlite3.h>
#include <array>
#include <cstdint>
#include <memory>

namespace ns::save {
namespace {
using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

SaveResult error(sqlite3 *db, const char *context) {
    return {false, std::string(context) + ": " + (db ? sqlite3_errmsg(db) : "cannot allocate database")};
}
bool exec(sqlite3 *db, const char *sql) { return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK; }
Statement prepare(sqlite3 *db, const char *sql) {
    sqlite3_stmt *raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
        return Statement(nullptr, sqlite3_finalize);
    }
    return Statement(raw, sqlite3_finalize);
}
int version(sqlite3 *db) {
    auto stmt = prepare(db, "PRAGMA user_version");
    return stmt && sqlite3_step(stmt.get()) == SQLITE_ROW ? sqlite3_column_int(stmt.get(), 0) : -1;
}
std::array<unsigned char, 8> encode(std::uint64_t value) {
    std::array<unsigned char, 8> result{};
    for (unsigned i = 0; i < 8; ++i) result[i] = static_cast<unsigned char>(value >> (i * 8U));
    return result;
}
std::uint64_t decode(const unsigned char *data) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(data[i]) << (i * 8U);
    return value;
}
Database open(const std::filesystem::path &path, int flags) {
    sqlite3 *raw = nullptr;
    // SQLite's UTF-8 API accepts UTF-8 paths, including non-ASCII paths on Windows.
    const auto utf8 = path.u8string();
    sqlite3_open_v2(reinterpret_cast<const char *>(utf8.c_str()), &raw, flags, nullptr);
    return Database(raw, sqlite3_close);
}
} // namespace

SaveResult write(const std::filesystem::path &path, const Simulation &simulation) {
    auto db = open(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    if (!db || sqlite3_errcode(db.get()) != SQLITE_OK) return error(db.get(), "open save");
    if (!exec(db.get(), "BEGIN IMMEDIATE")) return error(db.get(), "begin save");
    const auto fail = [&](const char *context) {
        auto result = error(db.get(), context);
        exec(db.get(), "ROLLBACK");
        return result;
    };
    const int schema = version(db.get());
    if (schema == 0) {
        // Do not adopt an unrelated unversioned SQLite database.
        auto count = prepare(db.get(), "SELECT count(*) FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'");
        if (!count || sqlite3_step(count.get()) != SQLITE_ROW || sqlite3_column_int(count.get(), 0) != 0) {
            exec(db.get(), "ROLLBACK");
            return {false, "unversioned database is not empty"};
        }
        count.reset();
        if (!exec(db.get(), "CREATE TABLE snapshot (id INTEGER PRIMARY KEY CHECK(id=1), tick BLOB NOT NULL, credits INTEGER NOT NULL, rng BLOB NOT NULL); PRAGMA user_version=1"))
            return fail("create save schema");
    } else if (schema != 1) {
        exec(db.get(), "ROLLBACK");
        return {false, "unsupported save schema version"};
    }
    auto stmt = prepare(db.get(), "INSERT OR REPLACE INTO snapshot(id,tick,credits,rng) VALUES(1,?,?,?)");
    if (!stmt) return fail("prepare snapshot");
    const auto state = simulation.snapshot();
    const auto tick = encode(state.tick);
    const auto rng = encode(state.rngState);
    if (sqlite3_bind_blob(stmt.get(), 1, tick.data(), 8, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_int64(stmt.get(), 2, state.credits) != SQLITE_OK ||
        sqlite3_bind_blob(stmt.get(), 3, rng.data(), 8, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_step(stmt.get()) != SQLITE_DONE) return fail("write snapshot");
    stmt.reset();
    if (!exec(db.get(), "COMMIT")) return fail("commit save");
    return {true, "ok"};
}

SaveResult load(const std::filesystem::path &path, Simulation &simulation) {
    auto db = open(path, SQLITE_OPEN_READONLY);
    if (!db || sqlite3_errcode(db.get()) != SQLITE_OK) return error(db.get(), "open save");
    if (!exec(db.get(), "BEGIN")) return error(db.get(), "begin load");
    if (version(db.get()) != 1) return {false, "unsupported save schema version"};
    auto stmt = prepare(db.get(), "SELECT tick,credits,rng FROM snapshot WHERE id=1");
    if (!stmt || sqlite3_step(stmt.get()) != SQLITE_ROW) return {false, "missing or unreadable snapshot"};
    if (sqlite3_column_type(stmt.get(), 0) != SQLITE_BLOB || sqlite3_column_bytes(stmt.get(), 0) != 8 ||
        sqlite3_column_type(stmt.get(), 1) != SQLITE_INTEGER || sqlite3_column_type(stmt.get(), 2) != SQLITE_BLOB ||
        sqlite3_column_bytes(stmt.get(), 2) != 8) return {false, "invalid snapshot fields"};
    Snapshot state{decode(static_cast<const unsigned char *>(sqlite3_column_blob(stmt.get(), 0))),
                   sqlite3_column_int64(stmt.get(), 1),
                   decode(static_cast<const unsigned char *>(sqlite3_column_blob(stmt.get(), 2)))};
    stmt.reset();
    if (!exec(db.get(), "COMMIT")) return error(db.get(), "commit load");
    const auto result = simulation.restore(state);
    return {result.ok, std::string(result.message)};
}
} // namespace ns::save
