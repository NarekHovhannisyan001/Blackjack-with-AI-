#pragma once

#include <string>

/// Plain data transfer object holding a single AI training snapshot.
/// Stored in training_stats table for post-training analysis.
struct TrainingRecord {
    int         sessionId     = 0;
    std::string timestamp;
    std::string agentType;      // "QLearning" or "MonteCarlo"
    int         totalEpisodes  = 0;
    double      winRate        = 0.0;
    double      avgReward      = 0.0;
    double      epsilon        = 0.0;   // exploration rate at snapshot time
    int         qTableSize     = 0;     // number of entries in q_table at snapshot
};
