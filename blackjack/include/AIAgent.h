#pragma once

#include "Experience.h"
#include "Types.h"
#include <memory>
#include <vector>

class Database;

/// Abstract base class for all AI agents (Q-Learning, Monte Carlo).
/// Subclasses implement the specific learning algorithm.
/// Intended for inheritance — virtual destructor required.
class AIAgent {
public:
    virtual ~AIAgent() = default;

    /// Choose an action for the given state.
    /// Training mode: epsilon-greedy (explore vs exploit).
    /// Evaluation mode: pure exploitation (epsilon = 0).
    /// @throws std::logic_error if state fields are out of valid range.
    virtual PlayerAction decide(const GameStateSnapshot& state) = 0;

    /// Update policy from one experience transition (Q-Learning).
    /// Monte Carlo agents implement this as a no-op and use learnEpisode().
    virtual void learn(const Experience& exp) = 0;

    /// Update policy from a complete episode buffer (Monte Carlo).
    /// Q-Learning agents implement this as a no-op.
    virtual void learnEpisode(const std::vector<Experience>& episode) = 0;

    /// Switch between Training and Evaluation modes.
    virtual void setMode(AgentMode mode) = 0;

    virtual AgentMode getMode() const = 0;

    /// Persist the current policy to the database.
    virtual void savePolicy(Database& db) = 0;

    /// Load policy from the database.
    /// @return false if no data was found (first run — policy stays default).
    virtual bool loadPolicy(const Database& db) = 0;

    /// Total number of policy updates applied so far.
    virtual int updateCount() const = 0;

    /// Current exploration rate. Q-Learning returns its decaying epsilon;
    /// Monte Carlo returns its fixed exploration rate.
    virtual double currentEpsilon() const = 0;
};
