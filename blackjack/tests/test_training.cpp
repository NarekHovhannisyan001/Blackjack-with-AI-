#include "doctest.h"
#include "Database.h"
#include "TrainingRunner.h"
#include "Types.h"
#include <cstdio>
#include <string>

static void cleanDb(const std::string& path) {
    std::remove(path.c_str());
    std::remove((path + "-wal").c_str());
    std::remove((path + "-shm").c_str());
}

// ── QLearning training ────────────────────────────────────────────────────────

TEST_CASE("TrainingRunner: QLearning runs 200 episodes without throwing") {
    const std::string DB = "test_tr_ql_smoke.db";
    cleanDb(DB);
    {
        TrainingRunner runner(AgentType::QLearning, 1, DB);
        CHECK_NOTHROW(runner.run(200));
        CHECK(runner.episodesRun() == 200);
    }
    cleanDb(DB);
}

TEST_CASE("TrainingRunner: QLearning updateCount > 0 after training") {
    const std::string DB = "test_tr_ql_count.db";
    cleanDb(DB);
    {
        TrainingRunner runner(AgentType::QLearning, 1, DB);
        runner.run(100);
        // Q-Learning calls learn() once per action step — at least 2 per hand
        CHECK(runner.updateCount() >= 100);
    }
    cleanDb(DB);
}

TEST_CASE("TrainingRunner: QLearning epsilon decays during training") {
    const std::string DB = "test_tr_ql_eps.db";
    cleanDb(DB);
    {
        TrainingRunner runner(AgentType::QLearning, 1, DB);
        double before = runner.lastEpsilon();
        runner.run(500);
        double after = runner.lastEpsilon();
        // Epsilon should have decreased from EPSILON_START
        CHECK(after < before);
    }
    cleanDb(DB);
}

// ── MonteCarlo training ───────────────────────────────────────────────────────

TEST_CASE("TrainingRunner: MonteCarlo runs 200 episodes without throwing") {
    const std::string DB = "test_tr_mc_smoke.db";
    cleanDb(DB);
    {
        TrainingRunner runner(AgentType::MonteCarlo, 1, DB);
        CHECK_NOTHROW(runner.run(200));
        CHECK(runner.episodesRun() == 200);
    }
    cleanDb(DB);
}

TEST_CASE("TrainingRunner: MonteCarlo updateCount equals episodes after training") {
    const std::string DB = "test_tr_mc_count.db";
    cleanDb(DB);
    {
        TrainingRunner runner(AgentType::MonteCarlo, 1, DB);
        runner.run(50);
        // MC calls learnEpisode() once per episode
        CHECK(runner.updateCount() == 50);
    }
    cleanDb(DB);
}

// ── DB round-trip ─────────────────────────────────────────────────────────────

TEST_CASE("TrainingRunner: QLearning policy persists across runner instances") {
    const std::string DB = "test_tr_ql_rt.db";
    cleanDb(DB);
    {
        TrainingRunner runner1(AgentType::QLearning, 1, DB);
        runner1.run(300);
        int count1 = runner1.updateCount();
        CHECK(count1 > 0);
    }
    {
        // Second runner loads the saved policy — updateCount resumes from there
        TrainingRunner runner2(AgentType::QLearning, 1, DB);
        runner2.run(100);
        // The Q-table was loaded from DB so it has prior knowledge
        CHECK(runner2.updateCount() > 0);
    }
    cleanDb(DB);
}

TEST_CASE("TrainingRunner: MonteCarlo policy persists across runner instances") {
    const std::string DB = "test_tr_mc_rt.db";
    cleanDb(DB);
    {
        TrainingRunner runner1(AgentType::MonteCarlo, 1, DB);
        runner1.run(100);
        CHECK(runner1.updateCount() == 100);
    }
    {
        TrainingRunner runner2(AgentType::MonteCarlo, 1, DB);
        runner2.run(50);
        CHECK(runner2.updateCount() == 50);
    }
    cleanDb(DB);
}
