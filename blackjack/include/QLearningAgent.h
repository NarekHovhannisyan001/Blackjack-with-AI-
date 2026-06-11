#pragma once

#include "AIAgent.h"
#include "Database.h"
#include "PolicyManager.h"
#include "StateEncoder.h"
#include "Types.h"
#include <random>
#include <string>

/// Q-Learning agent: off-policy TD control.
/// Updates after every step via the Bellman equation.
/// Uses epsilon-greedy exploration with exponential decay managed by PolicyManager.
class QLearningAgent : public AIAgent {
public:
    explicit QLearningAgent(double alpha = ALPHA, double gamma = GAMMA);

    /// Choose action via epsilon-greedy (training) or pure exploitation (eval).
    /// @throws std::logic_error if state is invalid (propagated from StateEncoder).
    PlayerAction decide(const GameStateSnapshot& state) override;

    /// Apply one-step Q-Learning update:
    ///   Q(s,a) += alpha * (reward + gamma * max_Q(s') - Q(s,a))
    void learn(const Experience& exp) override;

    /// No-op — Q-Learning updates per step, not per episode.
    void learnEpisode(const std::vector<Experience>&) override {}

    void      setMode(AgentMode mode) override;
    AgentMode getMode()               const override;

    void savePolicy(Database& db)       override;
    bool loadPolicy(const Database& db) override;

    int    updateCount()    const override;
    double currentEpsilon() const override;

private:
    QTableMap     m_qTable;
    StateEncoder  m_encoder;
    PolicyManager m_policyManager;
    AgentMode     m_mode        = AgentMode::Training;
    int           m_updateCount = 0;
    double        m_alpha;
    double        m_gamma;
    std::mt19937  m_rng;

    double       getQ(const std::string& stateKey, PlayerAction action) const;
    void         setQ(const std::string& stateKey, PlayerAction action, double value);
    PlayerAction bestAction(const std::string& stateKey) const;
    double       maxQ(const std::string& stateKey) const;
    PlayerAction randomValidAction(const GameStateSnapshot& state);
};
