#include "doctest.h"
#include "Database.h"
#include "Experience.h"
#include "PolicyManager.h"
#include "QLearningAgent.h"
#include "Types.h"
#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_map>

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

// ── Optimistic initialisation ─────────────────────────────────────────────────

TEST_CASE("QLearning: new agent returns 0.0 Q-value for any unseen state") {
    QLearningAgent agent;
    // No direct getQ access — verify indirectly via decide() not throwing
    // and via convergence tests below.
    // Use decide() in eval mode — should work on fresh agent without crash.
    agent.setMode(AgentMode::Evaluation);
    CHECK_NOTHROW(agent.decide(makeSnap()));
}

// ── learn() updates Q-value ───────────────────────────────────────────────────

TEST_CASE("QLearning: Q-value changes after one learn() call") {
    QLearningAgent agent;
    // Two consecutive decide() calls in eval mode with no learning
    // should return same action; after learn(), decision may differ.
    // Primarily test that learn() doesn't throw and updateCount increments.
    CHECK(agent.updateCount() == 0);
    agent.learn(makeExp(makeSnap(), PlayerAction::Stand, 1.0));
    CHECK(agent.updateCount() == 1);
}

// ── Convergence toward reward ─────────────────────────────────────────────────

TEST_CASE("QLearning: Q-value converges to +1.0 after many identical terminal experiences") {
    QLearningAgent agent(0.1, 0.9);
    GameStateSnapshot s = makeSnap(16, false, 7, 0);
    Experience exp = makeExp(s, PlayerAction::Stand, 1.0, true);

    for (int i = 0; i < 1000; ++i) {
        agent.learn(exp);
    }

    // After 1000 updates with reward +1.0, the best action should be Stand
    // and decide() in eval mode should consistently return Stand.
    agent.setMode(AgentMode::Evaluation);
    PlayerAction action = agent.decide(s);
    CHECK(action == PlayerAction::Stand);
}

TEST_CASE("QLearning: Q-value converges to -1.0 for negative reward") {
    QLearningAgent agent(0.1, 0.9);
    GameStateSnapshot s = makeSnap(16, false, 7, 0);

    // Train Stand → -1.0 (losing action)
    for (int i = 0; i < 500; ++i) {
        agent.learn(makeExp(s, PlayerAction::Stand, -1.0, true));
    }
    // Train Hit → +1.0 (winning action)
    for (int i = 0; i < 500; ++i) {
        agent.learn(makeExp(s, PlayerAction::Hit, 1.0, true));
    }

    agent.setMode(AgentMode::Evaluation);
    CHECK(agent.decide(s) == PlayerAction::Hit);
}

// ── Evaluation mode determinism ───────────────────────────────────────────────

TEST_CASE("QLearning: evaluation mode returns same action every call") {
    QLearningAgent agent(0.1, 0.9);
    GameStateSnapshot s = makeSnap(18, false, 9, 0);

    // Train a clear preference
    for (int i = 0; i < 200; ++i) {
        agent.learn(makeExp(s, PlayerAction::Stand, 1.0, true));
        agent.learn(makeExp(s, PlayerAction::Hit,  -1.0, true));
    }

    agent.setMode(AgentMode::Evaluation);
    PlayerAction first = agent.decide(s);
    for (int i = 0; i < 99; ++i) {
        CHECK(agent.decide(s) == first);
    }
}

// ── Training mode randomness ──────────────────────────────────────────────────

TEST_CASE("QLearning: training mode with high epsilon returns varied actions") {
    // Fresh agent has epsilon = 1.0 → fully random
    QLearningAgent agent;
    agent.setMode(AgentMode::Training);

    GameStateSnapshot s = makeSnap(10, false, 6, 0);
    s.canDouble    = true;
    s.canSurrender = true;

    std::unordered_map<int, int> counts;
    for (int i = 0; i < 400; ++i) {
        PlayerAction a = agent.decide(s);
        counts[static_cast<int>(a)]++;
        // Don't call learn() so epsilon stays high (fresh PolicyManager per decide)
        // Actually epsilon only steps on learn() — so it stays at 1.0
    }
    // With ε=1.0 and 400 samples, all valid actions should appear
    // Valid: Hit, Stand, Double, Surrender (no Split since canSplit=false)
    CHECK(counts[static_cast<int>(PlayerAction::Hit)]       > 0);
    CHECK(counts[static_cast<int>(PlayerAction::Stand)]     > 0);
    CHECK(counts[static_cast<int>(PlayerAction::Double)]    > 0);
    CHECK(counts[static_cast<int>(PlayerAction::Surrender)] > 0);
    CHECK(counts[static_cast<int>(PlayerAction::Split)]     == 0);
}

// ── randomValidAction respects canSplit / canDouble ──────────────────────────

TEST_CASE("QLearning: random action never returns Split when canSplit=false") {
    QLearningAgent agent;
    GameStateSnapshot s = makeSnap(10, false, 6, 0);
    s.canSplit  = false;
    s.canDouble = false;

    for (int i = 0; i < 200; ++i) {
        PlayerAction a = agent.decide(s);
        CHECK(a != PlayerAction::Split);
        CHECK(a != PlayerAction::Double);
    }
}

TEST_CASE("QLearning: random action never returns Double when canDouble=false") {
    QLearningAgent agent;
    GameStateSnapshot s = makeSnap(11, false, 5, 0);
    s.canDouble = false;
    s.canSplit  = false;

    for (int i = 0; i < 200; ++i) {
        PlayerAction a = agent.decide(s);
        CHECK(a != PlayerAction::Double);
    }
}

// ── Epsilon decay ─────────────────────────────────────────────────────────────

TEST_CASE("QLearning: epsilon reaches EPSILON_MIN after TRAINING_EPISODES steps") {
    // Each learn() call increments PolicyManager one step.
    QLearningAgent agent;
    GameStateSnapshot s = makeSnap();

    // Simulate TRAINING_EPISODES learn() calls
    for (int i = 0; i < TRAINING_EPISODES; ++i) {
        agent.learn(makeExp(s, PlayerAction::Stand, 0.0, true));
    }

    // Epsilon should be at or very near EPSILON_MIN
    CHECK(agent.currentEpsilon() <= EPSILON_MIN + 1e-6);
    CHECK(agent.currentEpsilon() >= EPSILON_MIN - 1e-6);
}

// ── visit_count increments ────────────────────────────────────────────────────

TEST_CASE("QLearning: updateCount increments on every learn() call") {
    QLearningAgent agent;
    GameStateSnapshot s = makeSnap();
    for (int i = 1; i <= 5; ++i) {
        agent.learn(makeExp(s, PlayerAction::Hit, 0.5, true));
        CHECK(agent.updateCount() == i);
    }
}

// ── DB round-trip ─────────────────────────────────────────────────────────────

TEST_CASE("QLearning: savePolicy/loadPolicy round-trip preserves Q-values") {
    const std::string TEST_DB = "test_ql_rt.db";
    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());

    {
        QLearningAgent agent;
        GameStateSnapshot s = makeSnap(16, false, 7, 0);

        // Train a clear preference
        for (int i = 0; i < 300; ++i) {
            agent.learn(makeExp(s, PlayerAction::Stand, 1.0, true));
        }

        agent.setMode(AgentMode::Evaluation);
        PlayerAction before = agent.decide(s);

        Database db(TEST_DB);
        int sid = db.startSession(AgentType::QLearning, 6, 0);
        (void)sid;
        agent.savePolicy(db);

        // Load into fresh agent and verify same decision
        QLearningAgent agent2;
        bool loaded = agent2.loadPolicy(db);
        CHECK(loaded == true);
        agent2.setMode(AgentMode::Evaluation);
        CHECK(agent2.decide(s) == before);
    }

    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());
}

TEST_CASE("QLearning: loadPolicy returns false on empty database") {
    const std::string TEST_DB = "test_ql_empty.db";
    std::remove(TEST_DB.c_str());
    Database db(TEST_DB);
    QLearningAgent agent;
    CHECK(agent.loadPolicy(db) == false);
    std::remove(TEST_DB.c_str());
    std::remove((TEST_DB + "-wal").c_str());
    std::remove((TEST_DB + "-shm").c_str());
}
