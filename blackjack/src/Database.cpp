#include "Database.h"
#include "SqliteWrappers.h"
#include "sqlite3.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string agentTypeToString(AgentType t) {
    switch (t) {
        case AgentType::Human:      return "Human";
        case AgentType::QLearning:  return "QLearning";
        case AgentType::MonteCarlo: return "MonteCarlo";
    }
    return "Human";
}

static void execOrThrow(sqlite3* db, const char* sql, const std::string& context) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error(context + ": " + msg);
    }
}

// ── Construction ──────────────────────────────────────────────────────────────

Database::Database(const std::string& path)
    : m_conn(std::make_unique<SqliteConnection>(path)) {
    enableWalMode();
    createTables();
}

Database::~Database() = default;

void Database::enableWalMode() {
    sqlite3* db = m_conn->get();
    execOrThrow(db, "PRAGMA journal_mode=WAL;",   "enableWalMode journal_mode");
    execOrThrow(db, "PRAGMA synchronous=NORMAL;", "enableWalMode synchronous");
    execOrThrow(db, "PRAGMA foreign_keys=ON;",    "enableWalMode foreign_keys");
}

void Database::createTables() {
    sqlite3* db = m_conn->get();

    execOrThrow(db, R"sql(
        CREATE TABLE IF NOT EXISTS game_sessions (
            session_id      INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp       TEXT    NOT NULL,
            agent_type      TEXT    NOT NULL,
            num_decks       INTEGER NOT NULL,
            starting_chips  INTEGER NOT NULL,
            final_chips     INTEGER DEFAULT 0,
            rounds_played   INTEGER DEFAULT 0,
            win_rate        REAL    DEFAULT 0.0
        );
    )sql", "createTables game_sessions");

    execOrThrow(db, R"sql(
        CREATE TABLE IF NOT EXISTS game_history (
            round_id            INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id          INTEGER NOT NULL,
            timestamp           TEXT    NOT NULL,
            player_hand         TEXT    NOT NULL,
            dealer_hand         TEXT    NOT NULL,
            action_taken        TEXT    NOT NULL,
            outcome             TEXT    NOT NULL,
            chips_before        INTEGER NOT NULL,
            chips_after         INTEGER NOT NULL,
            bet_amount          INTEGER NOT NULL,
            is_soft_hand        INTEGER NOT NULL,
            dealer_upcard_value INTEGER NOT NULL,
            FOREIGN KEY (session_id) REFERENCES game_sessions(session_id)
        );
    )sql", "createTables game_history");

    execOrThrow(db, R"sql(
        CREATE INDEX IF NOT EXISTS idx_history_session
            ON game_history(session_id);
    )sql", "createTables idx_history_session");

    execOrThrow(db, R"sql(
        CREATE TABLE IF NOT EXISTS training_stats (
            stat_id         INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id      INTEGER NOT NULL,
            timestamp       TEXT    NOT NULL,
            agent_type      TEXT    NOT NULL,
            total_episodes  INTEGER NOT NULL,
            win_rate        REAL    NOT NULL,
            avg_reward      REAL    NOT NULL,
            epsilon         REAL    NOT NULL,
            q_table_size    INTEGER NOT NULL,
            FOREIGN KEY (session_id) REFERENCES game_sessions(session_id)
        );
    )sql", "createTables training_stats");

    execOrThrow(db, R"sql(
        CREATE TABLE IF NOT EXISTS q_table (
            state_hash   TEXT    NOT NULL,
            action       TEXT    NOT NULL,
            q_value      REAL    NOT NULL DEFAULT 0.0,
            visit_count  INTEGER NOT NULL DEFAULT 0,
            last_updated TEXT    NOT NULL,
            PRIMARY KEY (state_hash, action)
        );
    )sql", "createTables q_table");

    execOrThrow(db, R"sql(
        CREATE TABLE IF NOT EXISTS mc_returns (
            state_hash   TEXT    NOT NULL,
            action       TEXT    NOT NULL,
            total_return REAL    NOT NULL DEFAULT 0.0,
            visit_count  INTEGER NOT NULL DEFAULT 0,
            mean_return  REAL    NOT NULL DEFAULT 0.0,
            last_updated TEXT    NOT NULL,
            PRIMARY KEY (state_hash, action)
        );
    )sql", "createTables mc_returns");
}

// ── Session management ────────────────────────────────────────────────────────

int Database::startSession(AgentType agentType, int numDecks, int startingChips) {
    const char* sql = R"sql(
        INSERT INTO game_sessions (timestamp, agent_type, num_decks, starting_chips)
        VALUES (?, ?, ?, ?);
    )sql";
    SqliteStatement stmt(m_conn->get(), sql);

    std::string ts   = currentTimestamp();
    std::string type = agentTypeToString(agentType);

    sqlite3_bind_text(stmt.get(), 1, ts.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt.get(), 3, numDecks);
    sqlite3_bind_int (stmt.get(), 4, startingChips);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(
            std::string("startSession: insert failed: ") + sqlite3_errmsg(m_conn->get()));
    }

    m_currentSessionId = static_cast<int>(sqlite3_last_insert_rowid(m_conn->get()));
    return m_currentSessionId;
}

void Database::endSession(int sessionId, int finalChips, int roundsPlayed, double winRate) {
    const char* sql = R"sql(
        UPDATE game_sessions
        SET final_chips = ?, rounds_played = ?, win_rate = ?
        WHERE session_id = ?;
    )sql";
    SqliteStatement stmt(m_conn->get(), sql);

    sqlite3_bind_int   (stmt.get(), 1, finalChips);
    sqlite3_bind_int   (stmt.get(), 2, roundsPlayed);
    sqlite3_bind_double(stmt.get(), 3, winRate);
    sqlite3_bind_int   (stmt.get(), 4, sessionId);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(
            std::string("endSession: update failed: ") + sqlite3_errmsg(m_conn->get()));
    }
}

// ── Batch writes ──────────────────────────────────────────────────────────────

void Database::insertRound(const GameRecord& record) {
    m_batchBuffer.push_back(record);
}

bool Database::batchReady() const {
    return static_cast<int>(m_batchBuffer.size()) >= BATCH_SIZE;
}

void Database::insertRoundRow(sqlite3_stmt* stmt, const GameRecord& record) {
    sqlite3_bind_int (stmt, 1,  record.sessionId);
    sqlite3_bind_text(stmt, 2,  record.timestamp.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3,  record.playerHand.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4,  record.dealerHand.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5,  record.actionTaken.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6,  record.outcome.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7,  record.chipsBefore);
    sqlite3_bind_int (stmt, 8,  record.chipsAfter);
    sqlite3_bind_int (stmt, 9,  record.betAmount);
    sqlite3_bind_int (stmt, 10, record.isSoftHand ? 1 : 0);
    sqlite3_bind_int (stmt, 11, record.dealerUpcardValue);
}

void Database::flushBatch() {
    if (m_batchBuffer.empty()) {
        return;
    }

    sqlite3* db = m_conn->get();
    execOrThrow(db, "BEGIN TRANSACTION;", "flushBatch BEGIN");

    const char* sql = R"sql(
        INSERT INTO game_history
            (session_id, timestamp, player_hand, dealer_hand, action_taken,
             outcome, chips_before, chips_after, bet_amount,
             is_soft_hand, dealer_upcard_value)
        VALUES (?,?,?,?,?,?,?,?,?,?,?)
    )sql";

    try {
        SqliteStatement stmt(db, sql);
        for (const auto& record : m_batchBuffer) {
            sqlite3_reset(stmt.get());
            insertRoundRow(stmt.get(), record);
            int rc = sqlite3_step(stmt.get());
            if (rc != SQLITE_DONE) {
                throw std::runtime_error(
                    std::string("flushBatch: INSERT failed: ") + sqlite3_errmsg(db));
            }
        }
        execOrThrow(db, "COMMIT;", "flushBatch COMMIT");
        m_batchBuffer.clear();
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

// ── History queries ───────────────────────────────────────────────────────────

std::vector<GameRecord> Database::queryHistory(int sessionId, int limit) const {
    std::string sql = R"sql(
        SELECT round_id, session_id, timestamp, player_hand, dealer_hand,
               action_taken, outcome, chips_before, chips_after,
               bet_amount, is_soft_hand, dealer_upcard_value
        FROM game_history
    )sql";
    if (sessionId != -1) {
        sql += " WHERE session_id = " + std::to_string(sessionId);
    }
    sql += " ORDER BY round_id DESC LIMIT " + std::to_string(limit) + ";";

    SqliteStatement stmt(m_conn->get(), sql);
    std::vector<GameRecord> results;

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        GameRecord r;
        r.roundId           = sqlite3_column_int (stmt.get(), 0);
        r.sessionId         = sqlite3_column_int (stmt.get(), 1);
        r.timestamp         = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        r.playerHand        = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        r.dealerHand        = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4));
        r.actionTaken       = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5));
        r.outcome           = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 6));
        r.chipsBefore       = sqlite3_column_int (stmt.get(), 7);
        r.chipsAfter        = sqlite3_column_int (stmt.get(), 8);
        r.betAmount         = sqlite3_column_int (stmt.get(), 9);
        r.isSoftHand        = sqlite3_column_int (stmt.get(), 10) != 0;
        r.dealerUpcardValue = sqlite3_column_int (stmt.get(), 11);
        results.push_back(r);
    }
    return results;
}

void Database::displayHistory(int sessionId, int limit) const {
    auto rows = queryHistory(sessionId, limit);

    std::string countSql = "SELECT COUNT(*), "
        "SUM(CASE WHEN outcome IN ('Win','Blackjack') THEN 1 ELSE 0 END) "
        "FROM game_history";
    if (sessionId != -1) {
        countSql += " WHERE session_id = " + std::to_string(sessionId);
    }
    countSql += ";";

    SqliteStatement countStmt(m_conn->get(), countSql);
    int totalRounds = 0;
    double winRate  = 0.0;
    if (sqlite3_step(countStmt.get()) == SQLITE_ROW) {
        totalRounds = sqlite3_column_int(countStmt.get(), 0);
        int wins    = sqlite3_column_int(countStmt.get(), 1);
        winRate     = totalRounds > 0 ? 100.0 * wins / totalRounds : 0.0;
    }

    std::string sessionLabel = sessionId == -1
        ? "all sessions"
        : "session " + std::to_string(sessionId);

    // U+2550 double horizontal, U+2500 single horizontal
    const std::string border  = "\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90";
    const std::string divider = "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80";

    std::cout << "\n" << border << "\n";
    std::cout << " Game history  (last " << limit << " rounds, " << sessionLabel << ")\n";
    std::cout << border << "\n";
    std::cout << std::left
              << std::setw(5)  << " Rnd"
              << std::setw(19) << " Player hand"
              << std::setw(14) << " Dealer hand"
              << std::setw(10) << " Action"
              << std::setw(10) << " Result"
              << " Chips\n";
    std::cout << divider << "\n";

    for (const auto& r : rows) {
        std::cout << std::left
                  << std::setw(5)  << (" " + std::to_string(r.roundId))
                  << std::setw(19) << (" " + r.playerHand)
                  << std::setw(14) << (" " + r.dealerHand)
                  << std::setw(10) << (" " + r.actionTaken)
                  << std::setw(10) << (" " + r.outcome)
                  << " " << r.chipsAfter << "\n";
    }

    std::cout << divider << "\n";
    std::cout << " Showing " << rows.size() << " of " << totalRounds
              << " rounds.  Win rate this session: "
              << std::fixed << std::setprecision(1) << winRate << "%\n";
    std::cout << border << "\n\n";
}

// ── AI policy persistence ─────────────────────────────────────────────────────

QTableMap Database::loadQTable() const {
    const char* sql = "SELECT state_hash, action, q_value, visit_count FROM q_table;";
    SqliteStatement stmt(m_conn->get(), sql);

    QTableMap result;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string key =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        key += "|";
        key += reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));

        QEntry entry;
        entry.qValue     = sqlite3_column_double(stmt.get(), 2);
        entry.visitCount = sqlite3_column_int   (stmt.get(), 3);
        result[key]      = entry;
    }
    return result;
}

void Database::saveQTable(const QTableMap& qtable) {
    sqlite3* db = m_conn->get();
    execOrThrow(db, "BEGIN TRANSACTION;", "saveQTable BEGIN");

    const char* sql = R"sql(
        INSERT OR REPLACE INTO q_table
            (state_hash, action, q_value, visit_count, last_updated)
        VALUES (?, ?, ?, ?, ?);
    )sql";

    try {
        SqliteStatement stmt(db, sql);
        std::string ts = currentTimestamp();

        for (const auto& kv : qtable) {
            auto sep = kv.first.rfind('|');
            std::string stateHash = kv.first.substr(0, sep);
            std::string action    = kv.first.substr(sep + 1);

            sqlite3_reset(stmt.get());
            sqlite3_bind_text  (stmt.get(), 1, stateHash.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text  (stmt.get(), 2, action.c_str(),    -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt.get(), 3, kv.second.qValue);
            sqlite3_bind_int   (stmt.get(), 4, kv.second.visitCount);
            sqlite3_bind_text  (stmt.get(), 5, ts.c_str(),        -1, SQLITE_TRANSIENT);

            int rc = sqlite3_step(stmt.get());
            if (rc != SQLITE_DONE) {
                throw std::runtime_error(
                    std::string("saveQTable: INSERT failed: ") + sqlite3_errmsg(db));
            }
        }
        execOrThrow(db, "COMMIT;", "saveQTable COMMIT");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

McReturnsMap Database::loadMcReturns() const {
    const char* sql =
        "SELECT state_hash, action, total_return, visit_count, mean_return FROM mc_returns;";
    SqliteStatement stmt(m_conn->get(), sql);

    McReturnsMap result;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string key =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        key += "|";
        key += reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));

        McEntry entry;
        entry.totalReturn = sqlite3_column_double(stmt.get(), 2);
        entry.visitCount  = sqlite3_column_int   (stmt.get(), 3);
        entry.meanReturn  = sqlite3_column_double(stmt.get(), 4);
        result[key]       = entry;
    }
    return result;
}

void Database::saveMcReturns(const McReturnsMap& mcReturns) {
    sqlite3* db = m_conn->get();
    execOrThrow(db, "BEGIN TRANSACTION;", "saveMcReturns BEGIN");

    const char* sql = R"sql(
        INSERT OR REPLACE INTO mc_returns
            (state_hash, action, total_return, visit_count, mean_return, last_updated)
        VALUES (?, ?, ?, ?, ?, ?);
    )sql";

    try {
        SqliteStatement stmt(db, sql);
        std::string ts = currentTimestamp();

        for (const auto& kv : mcReturns) {
            auto sep = kv.first.rfind('|');
            std::string stateHash = kv.first.substr(0, sep);
            std::string action    = kv.first.substr(sep + 1);

            sqlite3_reset(stmt.get());
            sqlite3_bind_text  (stmt.get(), 1, stateHash.c_str(),      -1, SQLITE_TRANSIENT);
            sqlite3_bind_text  (stmt.get(), 2, action.c_str(),         -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt.get(), 3, kv.second.totalReturn);
            sqlite3_bind_int   (stmt.get(), 4, kv.second.visitCount);
            sqlite3_bind_double(stmt.get(), 5, kv.second.meanReturn);
            sqlite3_bind_text  (stmt.get(), 6, ts.c_str(),             -1, SQLITE_TRANSIENT);

            int rc = sqlite3_step(stmt.get());
            if (rc != SQLITE_DONE) {
                throw std::runtime_error(
                    std::string("saveMcReturns: INSERT failed: ") + sqlite3_errmsg(db));
            }
        }
        execOrThrow(db, "COMMIT;", "saveMcReturns COMMIT");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

// ── Training stats ────────────────────────────────────────────────────────────

void Database::insertTrainingSnapshot(const TrainingRecord& record) {
    const char* sql = R"sql(
        INSERT INTO training_stats
            (session_id, timestamp, agent_type, total_episodes,
             win_rate, avg_reward, epsilon, q_table_size)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )sql";
    SqliteStatement stmt(m_conn->get(), sql);

    sqlite3_bind_int   (stmt.get(), 1, record.sessionId);
    sqlite3_bind_text  (stmt.get(), 2, record.timestamp.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt.get(), 3, record.agentType.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt.get(), 4, record.totalEpisodes);
    sqlite3_bind_double(stmt.get(), 5, record.winRate);
    sqlite3_bind_double(stmt.get(), 6, record.avgReward);
    sqlite3_bind_double(stmt.get(), 7, record.epsilon);
    sqlite3_bind_int   (stmt.get(), 8, record.qTableSize);

    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(
            std::string("insertTrainingSnapshot: INSERT failed: ") +
            sqlite3_errmsg(m_conn->get()));
    }
}

std::vector<TrainingRecord> Database::queryTrainingStats(int sessionId) const {
    const char* sql = R"sql(
        SELECT session_id, timestamp, agent_type, total_episodes,
               win_rate, avg_reward, epsilon, q_table_size
        FROM training_stats
        WHERE session_id = ?
        ORDER BY stat_id ASC;
    )sql";
    SqliteStatement stmt(m_conn->get(), sql);
    sqlite3_bind_int(stmt.get(), 1, sessionId);

    std::vector<TrainingRecord> results;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        TrainingRecord r;
        r.sessionId     = sqlite3_column_int   (stmt.get(), 0);
        r.timestamp     = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        r.agentType     = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        r.totalEpisodes = sqlite3_column_int   (stmt.get(), 3);
        r.winRate       = sqlite3_column_double(stmt.get(), 4);
        r.avgReward     = sqlite3_column_double(stmt.get(), 5);
        r.epsilon       = sqlite3_column_double(stmt.get(), 6);
        r.qTableSize    = sqlite3_column_int   (stmt.get(), 7);
        results.push_back(r);
    }
    return results;
}

// ── Maintenance ───────────────────────────────────────────────────────────────

void Database::vacuum() {
    execOrThrow(m_conn->get(), "VACUUM;", "vacuum");
}

int Database::totalRoundsStored() const {
    const char* sql = "SELECT COUNT(*) FROM game_history;";
    SqliteStatement stmt(m_conn->get(), sql);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0);
    }
    return 0;
}
