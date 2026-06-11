#pragma once

#include "Experience.h"
#include "Types.h"
#include <memory>
#include <string>
#include <vector>

class AIAgent;
class Database;
class Deck;

/// Headless training loop for AI agents.
/// Runs a simplified inner simulation (no ConsoleView, no FSM overhead)
/// to maximise throughput. Periodically saves the learned policy to SQLite.
class TrainingRunner {
public:
    TrainingRunner(AgentType type, int numDecks, const std::string& dbPath);

    // Destructor defined in .cpp where AIAgent and Database are complete types.
    ~TrainingRunner();

    /// Train for the given number of episodes (full hands).
    void run(int episodes = TRAINING_EPISODES);

    int    episodesRun()  const;
    int    updateCount()  const;
    double lastEpsilon()  const;

private:
    std::unique_ptr<AIAgent>   m_agent;
    std::unique_ptr<Database>  m_db;
    std::vector<Experience>    m_episodeBuf;
    int                        m_numDecks;
    int                        m_sessionId    = 0;
    int                        m_episodesRun  = 0;
    int                        m_runningCount = 0;

    // Simulate one hand, populate m_episodeBuf, call agent.learn / learnEpisode.
    // Returns the terminal reward for this hand.
    double simulateEpisode(Deck& deck);
};
