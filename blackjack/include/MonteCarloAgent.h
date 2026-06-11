#pragma once

#include "AIAgent.h"
#include "Database.h"
#include "StateEncoder.h"
#include "Types.h"
#include <random>
#include <string>

/// First-visit Monte Carlo agent.
/// Updates episodically — accumulates a complete episode, then applies
/// returns backward from the terminal state.
///
/// Uses a FIXED exploration rate (no epsilon decay). Decay is wrong for MC
/// because the agent is episodic and needs sustained exploration to keep
/// visiting states it hasn't seen recently. If epsilon decays to near zero
/// early, those Q-values stagnate and the policy degrades.
class MonteCarloAgent : public AIAgent {
public:
    explicit MonteCarloAgent(double explorationRate = 0.1);

    /// Choose action via fixed epsilon-greedy (training) or pure exploitation (eval).
    PlayerAction decide(const GameStateSnapshot& state) override;

    /// No-op — MC learns episodically, not per-step.
    void learn(const Experience&) override {}

    /// First-visit MC update: walk episode backward, accumulate returns,
    /// update each (state, action) pair the first time it appears.
    void learnEpisode(const std::vector<Experience>& episode) override;

    void      setMode(AgentMode mode) override;
    AgentMode getMode()               const override;

    void savePolicy(Database& db)       override;
    bool loadPolicy(const Database& db) override;

    int    updateCount()    const override;
    double currentEpsilon() const override { return m_explorationRate; }

private:
    McReturnsMap  m_returns;
    StateEncoder  m_encoder;
    AgentMode     m_mode            = AgentMode::Training;
    int           m_updateCount     = 0;
    std::mt19937  m_rng;
    double        m_explorationRate;

    PlayerAction bestAction(const std::string& stateKey) const;
    double       meanReturn(const std::string& stateKey, PlayerAction action) const;
    PlayerAction randomValidAction(const GameStateSnapshot& state);
};
