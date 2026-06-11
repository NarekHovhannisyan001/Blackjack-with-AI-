#include "TrainingRunner.h"
#include "Types.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <numDecks> <qlearning|montecarlo> [episodes]\n";
        return 1;
    }

    int numDecks = 0;
    try {
        numDecks = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "Invalid numDecks: " << argv[1] << "\n";
        return 1;
    }

    if (numDecks < 1 || numDecks > 8) {
        std::cerr << "numDecks must be 1-8\n";
        return 1;
    }

    std::string agentStr = argv[2];
    AgentType type;
    if (agentStr == "qlearning") {
        type = AgentType::QLearning;
    } else if (agentStr == "montecarlo") {
        type = AgentType::MonteCarlo;
    } else {
        std::cerr << "Unknown agent type '" << agentStr
                  << "'. Use 'qlearning' or 'montecarlo'.\n";
        return 1;
    }

    int episodes = TRAINING_EPISODES;
    if (argc >= 4) {
        try {
            episodes = std::stoi(argv[3]);
        } catch (...) {
            std::cerr << "Invalid episodes: " << argv[3] << "\n";
            return 1;
        }
    }

    std::cout << "Training " << agentStr << " agent for " << episodes
              << " episodes with " << numDecks << " deck(s)...\n";

    try {
        TrainingRunner runner(type, numDecks, DB_FILENAME);
        runner.run(episodes);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
