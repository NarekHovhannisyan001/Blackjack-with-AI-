#include "SqliteWrappers.h"
#include "sqlite3.h"
#include <stdexcept>

void SqliteConnection::Deleter::operator()(sqlite3* db) const {
    sqlite3_close(db);
}

SqliteConnection::SqliteConnection(const std::string& path) {
    sqlite3* raw = nullptr;
    int rc = sqlite3_open(path.c_str(), &raw);
    m_handle.reset(raw);
    if (rc != SQLITE_OK) {
        std::string msg = raw ? sqlite3_errmsg(raw) : "sqlite3_open failed";
        throw std::runtime_error("SqliteConnection: cannot open '" + path + "': " + msg);
    }
}

sqlite3* SqliteConnection::get() const {
    return m_handle.get();
}

void SqliteStatement::Deleter::operator()(sqlite3_stmt* s) const {
    sqlite3_finalize(s);
}

SqliteStatement::SqliteStatement(sqlite3* db, const std::string& sql) {
    sqlite3_stmt* raw = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw, nullptr);
    m_handle.reset(raw);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(
            std::string("SqliteStatement: prepare failed: ") + sqlite3_errmsg(db) +
            "\nSQL: " + sql);
    }
}

sqlite3_stmt* SqliteStatement::get() const {
    return m_handle.get();
}
