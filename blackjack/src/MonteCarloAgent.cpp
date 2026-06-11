#include "MonteCarloAgent.h"
#include <chrono>
#include <limits>
#include <string>
#include <unordered_set>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string actionStr(PlayerAction a) {
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

MonteCarloAgent::MonteCarloAgent(double explorationRate)
    : m_explorationRate(explorationRate)
    , m_rng(static_cast<uint32_t>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {}

// ── Private helpers ───────────────────────────────────────────────────────────

double MonteCarloAgent::meanReturn(const std::string& stateKey,
                                    PlayerAction action) const {
    auto it = m_returns.find(stateKey + "|" + actionStr(action));
    if (it == m_returns.end()) {
        return 0.0;
    }
    return it->second.meanReturn;
}

PlayerAction MonteCarloAgent::bestAction(const std::string& stateKey) const {
    PlayerAction best  = PlayerAction::Stand;
    double       bestR = std::numeric_limits<double>::lowest();
    for (PlayerAction a : ALL_ACTIONS) {
        double r = meanReturn(stateKey, a);
        if (r > bestR) {
            bestR = r;
            best  = a;
        }
    }
    return best;
}

PlayerAction MonteCarloAgent::randomValidAction(const GameStateSnapshot& state) {
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

PlayerAction MonteCarloAgent::decide(const GameStateSnapshot& state) {
    std::string key = m_encoder.encode(state); // throws if state invalid

    if (m_mode == AgentMode::Evaluation) {
        return bestAction(key);
    }

    // Fixed-rate epsilon-greedy (no decay)
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(m_rng) < m_explorationRate) {
        return randomValidAction(state);
    }
    return bestAction(key);
}

void MonteCarloAgent::learnEpisode(const std::vector<Experience>& episode) {
    if (episode.empty()) {
        return;
    }

    // First-visit MC: only update the first occurrence of each (state, action) pair.
    // Walking backward accumulates the discounted return G from the terminal state.
    std::unordered_set<std::string> visitedThisEpisode;
    visitedThisEpisode.reserve(episode.size() * 2);

    double G = 0.0;
    for (int i = static_cast<int>(episode.size()) - 1; i >= 0; --i) {
        const Experience& exp = episode[i];
        G = exp.reward + GAMMA * G;

        std::string key      = m_encoder.encode(exp.state);
        std::string mapKey   = key + "|" + actionStr(exp.action);
        std::string visitKey = mapKey; // same string reused as visit marker

        if (visitedThisEpisode.count(visitKey) == 0) {
            visitedThisEpisode.insert(visitKey);

            McEntry& entry = m_returns[mapKey];
            entry.visitCount++;
            entry.totalReturn += G;
            entry.meanReturn   = entry.totalReturn / entry.visitCount;
        }
    }
    ++m_updateCount;
}

void MonteCarloAgent::setMode(AgentMode mode) {
    m_mode = mode;
}

AgentMode MonteCarloAgent::getMode() const {
    return m_mode;
}

void MonteCarloAgent::savePolicy(Database& db) {
    db.saveMcReturns(m_returns);
}

bool MonteCarloAgent::loadPolicy(const Database& db) {
    McReturnsMap loaded = db.loadMcReturns();
    if (loaded.empty()) {
        return false;
    }
    m_returns = std::move(loaded);
    return true;
}

int MonteCarloAgent::updateCount() const {
    return m_updateCount;
}
