#pragma once

#include <memory>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

/// RAII owner for a sqlite3* connection handle.
/// Non-copyable. Closes the connection on destruction via sqlite3_close().
class SqliteConnection {
public:
    /// Opens the database at the given path. Use ":memory:" for in-memory DB.
    /// @throws std::runtime_error if sqlite3_open fails.
    explicit SqliteConnection(const std::string& path);

    /// Returns the raw sqlite3* handle for use in sqlite3_* API calls.
    sqlite3* get() const;

    SqliteConnection(const SqliteConnection&)            = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;

private:
    struct Deleter {
        void operator()(sqlite3* db) const;
    };
    std::unique_ptr<sqlite3, Deleter> m_handle;
};

/// RAII owner for a sqlite3_stmt* prepared statement handle.
/// Non-copyable. Finalizes the statement on destruction via sqlite3_finalize().
class SqliteStatement {
public:
    /// Prepares the given SQL against the open connection.
    /// @throws std::runtime_error if sqlite3_prepare_v2 fails.
    explicit SqliteStatement(sqlite3* db, const std::string& sql);

    /// Returns the raw sqlite3_stmt* handle for use in sqlite3_* API calls.
    sqlite3_stmt* get() const;

    SqliteStatement(const SqliteStatement&)            = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;

private:
    struct Deleter {
        void operator()(sqlite3_stmt* s) const;
    };
    std::unique_ptr<sqlite3_stmt, Deleter> m_handle;
};
