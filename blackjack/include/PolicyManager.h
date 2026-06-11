#pragma once

#include "Types.h"
#include <memory>

class AIAgent;
class QLearningAgent;
class MonteCarloAgent;

/// Manages epsilon decay for Q-Learning and acts as a factory for AI agents.
class PolicyManager {
public:
    /// Computes decay rate so epsilon reaches EPSILON_MIN after totalEpisodes.
    explicit PolicyManager(int totalEpisodes = TRAINING_EPISODES);

    /// Current exploration rate (decays each stepEpisode() call).
    double currentEpsilon() const;

    /// Decay epsilon by one step (call once per training episode).
    /// Uses exponential decay: keeps exploration high early, tapers off.
    /// Linear decay is inferior — it spends too many late episodes near 0
    /// where the agent barely explores new states.
    void stepEpisode();

    /// Reset epsilon to EPSILON_START (use when restarting training).
    void resetEpsilon();

    int episodeCount() const;

    /// Factory: create the correct agent for the given type.
    /// @throws std::logic_error if type is AgentType::Human.
    static std::unique_ptr<AIAgent> createAgent(AgentType type);

    /// Head-to-head comparison result.
    struct ComparisonResult {
        double qlWinRate   = 0.0;
        double mcWinRate   = 0.0;
        int    gamesPlayed = 0;
    };

    /// Run evalGames games with each agent (epsilon=0, same deck seeds).
    /// Prints a formatted comparison table to std::cout.
    static ComparisonResult compare(QLearningAgent& ql,
                                    MonteCarloAgent& mc,
                                    int evalGames,
                                    int numDecks);

private:
    double m_epsilon      = EPSILON_START;
    int    m_episodeCount = 0;
    double m_decayRate    = 0.0;
};
