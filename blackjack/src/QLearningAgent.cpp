#include "QLearningAgent.h"
#include <chrono>
#include <limits>
#include <stdexcept>
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string actionKey(PlayerAction a) {
    switch (a) {
        case PlayerAction::Hit:       return "Hit";
        case PlayerAction::Stand:     return "Stand";
        case PlayerAction::Double:    return "Double";
        case PlayerAction::Split:     return "Split";
        case PlayerAction::Surrender: return "Surrender";
    }
    return "Stand";
}

static constexpr PlayerAction ALL_ACTIONS[] = {
    PlayerAction::Hit,
    PlayerAction::Stand,
    PlayerAction::Double,
    PlayerAction::Split,
    PlayerAction::Surrender
};

// ── Construction ──────────────────────────────────────────────────────────────

QLearningAgent::QLearningAgent(double alpha, double gamma)
    : m_alpha(alpha)
    , m_gamma(gamma)
    , m_rng(static_cast<uint32_t>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {}

// ── Private helpers ───────────────────────────────────────────────────────────

double QLearningAgent::getQ(const std::string& stateKey, PlayerAction action) const {
    auto it = m_qTable.find(stateKey + "|" + actionKey(action));
    if (it == m_qTable.end()) {
        return 0.0; // optimistic initialisation: unseen states start at 0
    }
    return it->second.qValue;
}

void QLearningAgent::setQ(const std::string& stateKey, PlayerAction action, double value) {
    auto& entry = m_qTable[stateKey + "|" + actionKey(action)];
    entry.qValue = value;
    entry.visitCount++;
}

PlayerAction QLearningAgent::bestAction(const std::string& stateKey) const {
    PlayerAction best   = PlayerAction::Stand;
    double       bestQ  = std::numeric_limits<double>::lowest();
    for (PlayerAction a : ALL_ACTIONS) {
        double q = getQ(stateKey, a);
        if (q > bestQ) {
            bestQ = q;
            best  = a;
        }
    }
    return best;
}

double QLearningAgent::maxQ(const std::string& stateKey) const {
    double best = std::numeric_limits<double>::lowest();
    for (PlayerAction a : ALL_ACTIONS) {
        double q = getQ(stateKey, a);
        if (q > best) { best = q; }
    }
    return best;
}

PlayerAction QLearningAgent::randomValidAction(const GameStateSnapshot& state) {
    // Build the set of legal actions for this state
    PlayerAction legal[5];
    int count = 0;
    legal[count++] = PlayerAction::Hit;
    legal[count++] = PlayerAction::Stand;
    if (state.canDouble)    { legal[count++] = PlayerAction::Double; }
    if (state.canSplit)     { legal[count++] = PlayerAction::Split; }
    if (state.canSurrender) { legal[count++] = PlayerAction::Surrender; }

    std::uniform_int_distribution<int> dist(0, count - 1);
    return legal[dist(m_rng)];
}

// ── AIAgent interface ─────────────────────────────────────────────────────────

PlayerAction QLearningAgent::decide(const GameStateSnapshot& state) {
    std::string key = m_encoder.encode(state); // throws if state invalid

    if (m_mode == AgentMode::Evaluation) {
        return bestAction(key);
    }

    // Epsilon-greedy
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(m_rng) < m_policyManager.currentEpsilon()) {
        return randomValidAction(state);
    }
    return bestAction(key);
}

void QLearningAgent::learn(const Experience& exp) {
    std::string s = m_encoder.encode(exp.state);

    double currentQ = getQ(s, exp.action);
    double target;
    if (exp.isTerminal) {
        target = exp.reward;
    } else {
        std::string s2 = m_encoder.encode(exp.nextState);
        target = exp.reward + m_gamma * maxQ(s2);
    }

    double newQ = currentQ + m_alpha * (target - currentQ);
    setQ(s, exp.action, newQ);
    ++m_updateCount;
    m_policyManager.stepEpisode();
}

void QLearningAgent::setMode(AgentMode mode) {
    m_mode = mode;
}

AgentMode QLearningAgent::getMode() const {
    return m_mode;
}

void QLearningAgent::savePolicy(Database& db) {
    db.saveQTable(m_qTable);
}

bool QLearningAgent::loadPolicy(const Database& db) {
    QTableMap loaded = db.loadQTable();
    if (loaded.empty()) {
        return false;
    }
    m_qTable = std::move(loaded);
    return true;
}

int QLearningAgent::updateCount() const {
    return m_updateCount;
}

double QLearningAgent::currentEpsilon() const {
    return m_policyManager.currentEpsilon();
}
