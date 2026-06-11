#pragma once

#include "GameRecord.h"
#include "TrainingRecord.h"
#include "Types.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SqliteConnection;
struct sqlite3_stmt;

/// Single entry in the Q-table: action-value estimate and visit count.
struct QEntry {
    double qValue     = 0.0;
    int    visitCount = 0;
};

/// Single entry in the MC returns table.
struct McEntry {
    double totalReturn = 0.0;
    int    visitCount  = 0;
    double meanReturn  = 0.0;
};

/// Maps "stateHash|action" -> QEntry
using QTableMap    = std::unordered_map<std::string, QEntry>;

/// Maps "stateHash|action" -> McEntry
using McReturnsMap = std::unordered_map<std::string, McEntry>;

/// Main database interface. Owns the SQLite connection and all SQL logic.
/// sqlite3.h is NOT included in this header — all SQLite types are hidden
/// behind SqliteConnection (forward-declared above).
/// Non-copyable: the connection owns a file/socket resource.
class Database {
public:
    /// Opens (or creates) the database at the given path.
    /// Runs createTables(), enableWalMode(), and foreign_keys pragma.
    /// @throws std::runtime_error if the database cannot be opened or tables fail.
    explicit Database(const std::string& path = DB_FILENAME);

    ~Database();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // ── Session management ───────────────────────────────────────────────
    /// Inserts a new row in game_sessions and returns the new session ID.
    /// @throws std::runtime_error if the insert fails.
    int startSession(AgentType agentType, int numDecks, int startingChips);

    /// Updates the session row with end-of-game statistics.
    /// @throws std::runtime_error if the update fails.
    void endSession(int sessionId, int finalChips, int roundsPlayed, double winRate);

    // ── Round history (batched writes) ───────────────────────────────────
    /// Appends one record to the in-memory batch buffer. Never throws. Never writes to disk.
    void insertRound(const GameRecord& record);

    /// Writes all buffered records to disk in a single transaction.
    /// Clears the buffer on success.
    /// @throws std::runtime_error if any insert fails (transaction is rolled back).
    void flushBatch();

    /// Returns true when the buffer has reached BATCH_SIZE records.
    bool batchReady() const;

    // ── History queries ──────────────────────────────────────────────────
    /// Prints the last N rounds (for sessionId, or all sessions if sessionId == -1)
    /// to stdout in a formatted table.
    /// @throws std::runtime_error on SQL error.
    void displayHistory(int sessionId = -1, int limit = MAX_HISTORY_DISPLAY) const;

    /// Returns the last N GameRecords as a vector.
    /// @throws std::runtime_error on SQL error.
    std::vector<GameRecord> queryHistory(int sessionId = -1,
                                         int limit = MAX_HISTORY_DISPLAY) const;

    // ── AI policy persistence ────────────────────────────────────────────
    /// Loads the full q_table into memory. Returns an empty map on a fresh DB.
    /// @throws std::runtime_error if rows contain corrupt data.
    QTableMap loadQTable() const;

    /// Replaces all rows in q_table with the entries in qtable.
    /// Uses INSERT OR REPLACE to handle existing keys.
    /// @throws std::runtime_error on failure.
    void saveQTable(const QTableMap& qtable);

    /// Loads the full mc_returns table. Returns an empty map on a fresh DB.
    McReturnsMap loadMcReturns() const;

    /// Replaces all rows in mc_returns with the entries in mcReturns.
    /// @throws std::runtime_error on failure.
    void saveMcReturns(const McReturnsMap& mcReturns);

    // ── Training stats ───────────────────────────────────────────────────
    /// Inserts one training snapshot into training_stats.
    /// @throws std::runtime_error on failure.
    void insertTrainingSnapshot(const TrainingRecord& record);

    /// Returns all training snapshots for the given session.
    /// @throws std::runtime_error on SQL error.
    std::vector<TrainingRecord> queryTrainingStats(int sessionId) const;

    // ── Maintenance ──────────────────────────────────────────────────────
    /// Runs VACUUM to reclaim unused file space.
    void vacuum();

    /// Returns the total number of rows in game_history.
    int totalRoundsStored() const;

private:
    std::unique_ptr<SqliteConnection> m_conn;
    std::vector<GameRecord>           m_batchBuffer;
    int                               m_currentSessionId = 0;

    void createTables();
    void enableWalMode();

    /// Inserts a single GameRecord row using a pre-prepared statement.
    /// Used inside flushBatch()'s transaction loop.
    void insertRoundRow(sqlite3_stmt* stmt, const GameRecord& record);
};
