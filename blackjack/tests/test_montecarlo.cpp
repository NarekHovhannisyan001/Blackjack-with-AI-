#include "doctest.h"
#include "Database.h"
#include "Experience.h"
#include "MonteCarloAgent.h"
#include "Types.h"
#include <cstdio>
#include <string>
#include <vector>

static GameStateSnapshot makeSnap(int total = 16, bool soft = false,
                                   int upcard = 7, int count = 0) {
    GameStateSnapshot s;
    s.playerTotal       = total;
    s.isSoftHand        = soft;
    s.dealerUpcardValue = upcard;
    s.runningCount      = count;
    s.canDouble         = false;
    s.canSplit          = false;
    s.canSurrender      = false;
    return s;
}

static Experience makeExp(GameStateSnapshot state, PlayerAction action,
                           double reward, bool terminal = true) {
    Experience e;
    e.state      = state;
    e.action     = action;
    e.reward     = reward;
    e.nextState  = state;
    e.isTerminal = terminal;
    return e;
}

// ── Default state ─────────────────────────────────────────────────────────────

TEST_CASE("MonteCarlo: new agent decide() does not throw") {
    MonteCarloAgent agent;
    agent.setMode(AgentMode::Evaluation);
    CHECK_NOTHROW(agent.decide(makeSnap()));
}

// ── learnEpisode with single experience ──────────────────────────────────────

TEST_CASE("MonteCarlo: learnEpisode with one experience updates mean_return") {
    MonteCarloAgent agent;
    std::vector<Experience> ep = { makeExp(makeSnap(), PlayerAction::Stand, 1.0) };
    agent.learnEpisode(ep);
    CHECK(agent.updateCount() == 1);

    // In eval mode, Stand should now be preferred on that state
    agent.setMode(AgentMode::Evaluation);
    CHECK(agent.decide(makeSnap()) == PlayerAction::Stand);
}

// ── First-visit property ──────────────────────────────────────────────────────

TEST_CASE("MonteCarlo: first-visit — same (state,action) in one episode counted once") {
    MonteCarloAgent agent;
    GameStateSnapshot s = makeSnap(16, false, 7, 0);

    // Episode visits (s, Hit) twice — first-visit MC should only count it once
    std::vector<Experience> ep = {
        makeExp(s, PlayerAction::Hit, 0.0, false),   // first visit
        makeExp(s, PlayerAction::Hit, 1.0, true)     // second visit — ignored by first-visit
    };
    agent.learnEpisode(ep);

    // updateCount advances once per episode
    CHECK(agent.updateCount() == 1);
}

// ── Convergence toward actual mean ───────────────────────────────────────────

TEST_CASE("MonteCarlo: mean_return converges toward +1.0 after many episodes") {
    MonteCarloAgent agent;
    GameStateSnapshot s = makeSnap(18, false, 9, 0);

    for (int i = 0; i < 500; ++i) {
        std::vector<Experience> ep = { makeExp(s, PlayerAction::Stand, 1.0, true) };
        agent.learnEpisode(ep);
    }

    agent.setMode(AgentMode::Evaluation);
    CHECK(agent.decide(s) == PlayerAction::Stand);
}

TEST_CASE("MonteCarlo: mean_return separates winning from losing actions") {
    MonteCarloAgent agent(0.0); // 0 exploration so eval is deterministic

    GameStateSnapshot s = makeSnap(12, false, 6, 0);

    for (int i = 0; i < 300; ++i) {
        // Stand → win
        std::vector<Experience> ep = { makeExp(s, PlayerAction::Stand, 1.0, true) };
        agent.learnEpisode(ep);
        // Hit → lose
        std::vector<Experience> ep2 = { makeExp(s, PlayerAction::Hit, -1.0, true) };
        agent.learnEpisode(ep2);
    }

    agent.setMode(AgentMode::Evaluation);
    CHECK(agent.decide(s) == PlayerAction::Stand);
}

// ── Evaluation mode determinism ───────────────────────────────────────────────

TEST_CASE("MonteCarlo: evaluation mode is deterministic") {
    MonteCarloAgent agent;
    GameStateSnapshot s = makeSnap(20, false, 10, 0);

    for (int i = 0; i < 100; ++i) {
        std::vector<Experience> ep = { makeExp(s, PlayerAction::Stand, 1.0, true) };
        agent.learnEpisode(ep);
    }

    agent.setMode(AgentMode::Evaluation);
    PlayerAction first = agent.decide(s);
    for (int i = 0; i < 49; ++i) {
        CHECK(agent.decide(s) == first);
    }
}

// ── Empty episode ─────────────────────────────────────────────────────────────

TEST_CASE("MonteCarlo: learnEpisode with empty vector does not throw") {
    MonteCarloAgent agent;
    std::vector<Experience> empty;
    CHECK_NOTHROW(agent.learnEpisode(empty));
    CHECK(agent.updateCount() == 0);
}

// ── DB round-trip ─────────────────────────────────────────────────────────────

TEST_CASE("MonteCarlo: savePolicy/loadPolicy round-trip preserves decisions") {
    const std::string TEST_DB = "test_mc_rt.db";
    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());

    {
        MonteCarloAgent agent;
        GameStateSnapshot s = makeSnap(16, false, 7, 0);

        for (int i = 0; i < 200; ++i) {
            std::vector<Experience> ep = { makeExp(s, PlayerAction::Stand, 1.0, true) };
            agent.learnEpisode(ep);
            std::vector<Experience> ep2 = { makeExp(s, PlayerAction::Hit, -1.0, true) };
            agent.learnEpisode(ep2);
        }

        agent.setMode(AgentMode::Evaluation);
        PlayerAction before = agent.decide(s);

        Database db(TEST_DB);
        int sid = db.startSession(AgentType::MonteCarlo, 6, 0);
        (void)sid;
        agent.savePolicy(db);

        MonteCarloAgent agent2;
        bool loaded = agent2.loadPolicy(db);
        CHECK(loaded == true);
        agent2.setMode(AgentMode::Evaluation);
        CHECK(agent2.decide(s) == before);
    }

    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());
}

TEST_CASE("MonteCarlo: loadPolicy returns false on empty database") {
    const std::string TEST_DB = "test_mc_empty.db";
    std::remove(TEST_DB.c_str());
    Database db(TEST_DB);
    MonteCarloAgent agent;
    CHECK(agent.loadPolicy(db) == false);
    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());
}
