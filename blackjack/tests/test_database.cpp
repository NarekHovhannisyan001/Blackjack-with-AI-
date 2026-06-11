#include "doctest.h"
#include "Database.h"
#include "GameRecord.h"
#include "TrainingRecord.h"
#include "Types.h"
#include <cstdio>
#include <stdexcept>
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

static const std::string TEST_DB = "test_database_temp.db";

// Remove the temp database (and WAL/SHM side-files) between tests.
static void removeTestDb() {
    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());
}

static GameRecord makeRecord(int sessionId, int roundNum) {
    GameRecord r;
    r.sessionId         = sessionId;
    r.roundId           = 0; // assigned by DB
    r.timestamp         = "2024-01-01T00:00:00";
    r.playerHand        = "A\xe2\x99\xa0 K\xe2\x99\xa5";  // A♠ K♥
    r.dealerHand        = "7\xe2\x99\xa6 9\xe2\x99\xa3";  // 7♦ 9♣
    r.actionTaken       = "Stand";
    r.outcome           = roundNum % 3 == 0 ? "Win" : (roundNum % 3 == 1 ? "Lose" : "Push");
    r.chipsBefore       = 1000 + roundNum * 10;
    r.chipsAfter        = r.chipsBefore + (r.outcome == "Win" ? 10 : (r.outcome == "Lose" ? -10 : 0));
    r.betAmount         = 10;
    r.isSoftHand        = (roundNum % 2 == 0);
    r.dealerUpcardValue = 7;
    return r;
}

// ── Construction ──────────────────────────────────────────────────────────────

TEST_CASE("Database: constructor creates tables and WAL mode") {
    removeTestDb();
    {
        Database db(TEST_DB);
        // totalRoundsStored() works on a freshly created DB
        CHECK(db.totalRoundsStored() == 0);
    }
    removeTestDb();
}

TEST_CASE("Database: opening existing DB does not reset data") {
    removeTestDb();
    {
        Database db(TEST_DB);
        int sid = db.startSession(AgentType::Human, 1, 1000);
        GameRecord r = makeRecord(sid, 0);
        db.insertRound(r);
        db.flushBatch();
        CHECK(db.totalRoundsStored() == 1);
    }
    {
        // Re-open — row should persist
        Database db(TEST_DB);
        CHECK(db.totalRoundsStored() == 1);
    }
    removeTestDb();
}

// ── Session management ────────────────────────────────────────────────────────

TEST_CASE("Database: startSession returns positive incrementing IDs") {
    removeTestDb();
    Database db(TEST_DB);

    int s1 = db.startSession(AgentType::Human,      1, 1000);
    int s2 = db.startSession(AgentType::QLearning,  2, 500);
    int s3 = db.startSession(AgentType::MonteCarlo, 4, 2000);

    CHECK(s1 >= 1);
    CHECK(s2 == s1 + 1);
    CHECK(s3 == s2 + 1);

    removeTestDb();
}

TEST_CASE("Database: endSession does not throw") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);
    CHECK_NOTHROW(db.endSession(sid, 1200, 10, 0.6));
    removeTestDb();
}

// ── insertRound / flushBatch ──────────────────────────────────────────────────

TEST_CASE("Database: insertRound buffers without writing") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);

    db.insertRound(makeRecord(sid, 1));
    // Buffer holds 1 record but nothing flushed yet
    CHECK(db.totalRoundsStored() == 0);

    removeTestDb();
}

TEST_CASE("Database: flushBatch writes buffered records") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);

    for (int i = 0; i < 5; ++i) {
        db.insertRound(makeRecord(sid, i));
    }
    db.flushBatch();
    CHECK(db.totalRoundsStored() == 5);

    removeTestDb();
}

TEST_CASE("Database: flushBatch on empty buffer is a no-op") {
    removeTestDb();
    Database db(TEST_DB);
    CHECK_NOTHROW(db.flushBatch());
    CHECK(db.totalRoundsStored() == 0);
    removeTestDb();
}

TEST_CASE("Database: batchReady returns true at BATCH_SIZE") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);

    CHECK_FALSE(db.batchReady());
    for (int i = 0; i < BATCH_SIZE; ++i) {
        db.insertRound(makeRecord(sid, i));
    }
    CHECK(db.batchReady());

    removeTestDb();
}

TEST_CASE("Database: multiple flushBatch calls accumulate rows") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);

    for (int i = 0; i < 3; ++i) {
        db.insertRound(makeRecord(sid, i));
    }
    db.flushBatch();
    CHECK(db.totalRoundsStored() == 3);

    for (int i = 3; i < 7; ++i) {
        db.insertRound(makeRecord(sid, i));
    }
    db.flushBatch();
    CHECK(db.totalRoundsStored() == 7);

    removeTestDb();
}

// ── queryHistory ──────────────────────────────────────────────────────────────

TEST_CASE("Database: queryHistory returns correct count") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);

    for (int i = 0; i < 10; ++i) {
        db.insertRound(makeRecord(sid, i));
    }
    db.flushBatch();

    auto all = db.queryHistory(-1, 100);
    CHECK(all.size() == 10);

    auto limited = db.queryHistory(-1, 5);
    CHECK(limited.size() == 5);

    removeTestDb();
}

TEST_CASE("Database: queryHistory filtered by session") {
    removeTestDb();
    Database db(TEST_DB);
    int s1 = db.startSession(AgentType::Human, 1, 1000);
    int s2 = db.startSession(AgentType::Human, 1, 1000);

    for (int i = 0; i < 4; ++i) db.insertRound(makeRecord(s1, i));
    for (int i = 0; i < 6; ++i) db.insertRound(makeRecord(s2, i));
    db.flushBatch();

    auto rows1 = db.queryHistory(s1, 100);
    auto rows2 = db.queryHistory(s2, 100);
    CHECK(rows1.size() == 4);
    CHECK(rows2.size() == 6);

    removeTestDb();
}

TEST_CASE("Database: queryHistory round-trips all fields") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::Human, 1, 1000);

    GameRecord src = makeRecord(sid, 0);
    src.isSoftHand = true;
    src.dealerUpcardValue = 11;
    src.betAmount = 50;
    db.insertRound(src);
    db.flushBatch();

    auto rows = db.queryHistory(sid, 1);
    REQUIRE(rows.size() == 1);
    const auto& got = rows[0];
    CHECK(got.sessionId         == src.sessionId);
    CHECK(got.playerHand        == src.playerHand);
    CHECK(got.dealerHand        == src.dealerHand);
    CHECK(got.actionTaken       == src.actionTaken);
    CHECK(got.outcome           == src.outcome);
    CHECK(got.chipsBefore       == src.chipsBefore);
    CHECK(got.chipsAfter        == src.chipsAfter);
    CHECK(got.betAmount         == src.betAmount);
    CHECK(got.isSoftHand        == src.isSoftHand);
    CHECK(got.dealerUpcardValue == src.dealerUpcardValue);

    removeTestDb();
}

// ── Q-table persistence ───────────────────────────────────────────────────────

TEST_CASE("Database: saveQTable / loadQTable round-trip") {
    removeTestDb();
    Database db(TEST_DB);

    QTableMap original;
    original["state1|Hit"]   = {1.5,  10};
    original["state1|Stand"] = {-0.3, 5};
    original["state2|Hit"]   = {0.0,  0};

    db.saveQTable(original);
    auto loaded = db.loadQTable();

    CHECK(loaded.size() == original.size());
    for (const auto& kv : original) {
        REQUIRE(loaded.count(kv.first) == 1);
        CHECK(loaded[kv.first].qValue     == doctest::Approx(kv.second.qValue));
        CHECK(loaded[kv.first].visitCount == kv.second.visitCount);
    }

    removeTestDb();
}

TEST_CASE("Database: saveQTable overwrites existing entries") {
    removeTestDb();
    Database db(TEST_DB);

    QTableMap v1;
    v1["s|Hit"] = {1.0, 1};
    db.saveQTable(v1);

    QTableMap v2;
    v2["s|Hit"] = {2.5, 7};
    db.saveQTable(v2);

    auto loaded = db.loadQTable();
    REQUIRE(loaded.count("s|Hit") == 1);
    CHECK(loaded["s|Hit"].qValue     == doctest::Approx(2.5));
    CHECK(loaded["s|Hit"].visitCount == 7);

    removeTestDb();
}

TEST_CASE("Database: loadQTable returns empty map on fresh DB") {
    removeTestDb();
    Database db(TEST_DB);
    auto loaded = db.loadQTable();
    CHECK(loaded.empty());
    removeTestDb();
}

// ── MC returns persistence ────────────────────────────────────────────────────

TEST_CASE("Database: saveMcReturns / loadMcReturns round-trip") {
    removeTestDb();
    Database db(TEST_DB);

    McReturnsMap original;
    original["stateA|Stand"] = {10.0, 5, 2.0};
    original["stateA|Hit"]   = {-3.0, 3, -1.0};

    db.saveMcReturns(original);
    auto loaded = db.loadMcReturns();

    CHECK(loaded.size() == original.size());
    for (const auto& kv : original) {
        REQUIRE(loaded.count(kv.first) == 1);
        CHECK(loaded[kv.first].totalReturn == doctest::Approx(kv.second.totalReturn));
        CHECK(loaded[kv.first].visitCount  == kv.second.visitCount);
        CHECK(loaded[kv.first].meanReturn  == doctest::Approx(kv.second.meanReturn));
    }

    removeTestDb();
}

TEST_CASE("Database: loadMcReturns returns empty map on fresh DB") {
    removeTestDb();
    Database db(TEST_DB);
    auto loaded = db.loadMcReturns();
    CHECK(loaded.empty());
    removeTestDb();
}

// ── Training stats ────────────────────────────────────────────────────────────

TEST_CASE("Database: insertTrainingSnapshot / queryTrainingStats round-trip") {
    removeTestDb();
    Database db(TEST_DB);
    int sid = db.startSession(AgentType::QLearning, 1, 500);

    TrainingRecord snap;
    snap.sessionId     = sid;
    snap.timestamp     = "2024-06-01T12:00:00";
    snap.agentType     = "QLearning";
    snap.totalEpisodes = 1000;
    snap.winRate       = 0.45;
    snap.avgReward     = 0.12;
    snap.epsilon       = 0.05;
    snap.qTableSize    = 256;

    db.insertTrainingSnapshot(snap);

    auto rows = db.queryTrainingStats(sid);
    REQUIRE(rows.size() == 1);
    CHECK(rows[0].sessionId     == snap.sessionId);
    CHECK(rows[0].agentType     == snap.agentType);
    CHECK(rows[0].totalEpisodes == snap.totalEpisodes);
    CHECK(rows[0].winRate       == doctest::Approx(snap.winRate));
    CHECK(rows[0].avgReward     == doctest::Approx(snap.avgReward));
    CHECK(rows[0].epsilon       == doctest::Approx(snap.epsilon));
    CHECK(rows[0].qTableSize    == snap.qTableSize);

    removeTestDb();
}

TEST_CASE("Database: queryTrainingStats empty for unknown session") {
    removeTestDb();
    Database db(TEST_DB);
    auto rows = db.queryTrainingStats(9999);
    CHECK(rows.empty());
    removeTestDb();
}

// ── Maintenance ───────────────────────────────────────────────────────────────

TEST_CASE("Database: vacuum does not throw") {
    removeTestDb();
    Database db(TEST_DB);
    CHECK_NOTHROW(db.vacuum());
    removeTestDb();
}

TEST_CASE("Database: totalRoundsStored counts across sessions") {
    removeTestDb();
    Database db(TEST_DB);
    int s1 = db.startSession(AgentType::Human, 1, 1000);
    int s2 = db.startSession(AgentType::Human, 1, 1000);

    for (int i = 0; i < 3; ++i) db.insertRound(makeRecord(s1, i));
    for (int i = 0; i < 5; ++i) db.insertRound(makeRecord(s2, i));
    db.flushBatch();

    CHECK(db.totalRoundsStored() == 8);
    removeTestDb();
}
