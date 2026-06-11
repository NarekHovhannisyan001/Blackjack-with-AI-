#include "PolicyManager.h"
#include "AIAgent.h"
#include "Deck.h"
#include "Dealer.h"
#include "Hand.h"
#include "MonteCarloAgent.h"
#include "QLearningAgent.h"
#include "StateEncoder.h"
#include "Types.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>

PolicyManager::PolicyManager(int totalEpisodes)
    : m_epsilon(EPSILON_START)
    , m_episodeCount(0) {
    m_decayRate = std::pow(EPSILON_MIN / EPSILON_START,
                           1.0 / static_cast<double>(totalEpisodes));
}

double PolicyManager::currentEpsilon() const {
    return m_epsilon;
}

void PolicyManager::stepEpisode() {
    m_epsilon = std::max(EPSILON_MIN, m_epsilon * m_decayRate);
    ++m_episodeCount;
}

void PolicyManager::resetEpsilon() {
    m_epsilon      = EPSILON_START;
    m_episodeCount = 0;
}

int PolicyManager::episodeCount() const {
    return m_episodeCount;
}

std::unique_ptr<AIAgent> PolicyManager::createAgent(AgentType type) {
    if (type == AgentType::QLearning) {
        return std::make_unique<QLearningAgent>();
    }
    if (type == AgentType::MonteCarlo) {
        return std::make_unique<MonteCarloAgent>();
    }
    throw std::logic_error(
        "PolicyManager::createAgent: Human is not a trainable agent type");
}

// ── Head-to-head comparison ───────────────────────────────────────────────────

// Simulate one complete hand headlessly, returns true if player beat dealer.
// Simplified: player uses basic-ish logic via agent.decide(), dealer hits <= 16.
static bool runOneGame(AIAgent& agent, Deck& deck) {
    // Deal two cards each
    Hand playerHand, dealerHand;
    playerHand.addCard(deck.deal());
    dealerHand.addCard(deck.deal());
    playerHand.addCard(deck.deal());
    dealerHand.addCard(deck.deal());

    int dealerUpcard = dealerHand.getCards()[0].getValue();

    // Player acts until stand/bust
    while (!playerHand.isBust()) {
        GameStateSnapshot snap;
        snap.playerTotal       = playerHand.getValue();
        snap.isSoftHand        = playerHand.isSoft();
        snap.dealerUpcardValue = dealerUpcard;
        snap.runningCount      = 0;
        snap.canDouble         = false;
        snap.canSplit          = false;
        snap.canSurrender      = false;

        // Clamp to valid range before calling decide
        if (snap.playerTotal < 4)  { break; }
        if (snap.playerTotal > 21) { break; }

        PlayerAction action = agent.decide(snap);
        if (action == PlayerAction::Stand   ||
            action == PlayerAction::Double  ||
            action == PlayerAction::Surrender) {
            break;
        }
        // Hit or Split treated as Hit for comparison
        playerHand.addCard(deck.deal());
    }

    if (playerHand.isBust()) { return false; }

    // Dealer hits to 17+
    while (dealerHand.getValue() < 17 ||
           (dealerHand.isSoft() && dealerHand.getValue() == 17)) {
        dealerHand.addCard(deck.deal());
    }

    if (dealerHand.isBust())                           { return true; }
    if (playerHand.getValue() > dealerHand.getValue()) { return true; }
    return false;
}

PolicyManager::ComparisonResult PolicyManager::compare(
    QLearningAgent& ql, MonteCarloAgent& mc, int evalGames, int numDecks) {

    ql.setMode(AgentMode::Evaluation);
    mc.setMode(AgentMode::Evaluation);

    int qlWins = 0;
    int mcWins = 0;

    for (int i = 0; i < evalGames; ++i) {
        Deck deck(numDecks);
        deck.shuffle(static_cast<uint32_t>(i + 1)); // same seed for both agents

        // Both agents play against the same pre-shuffled shoe
        bool qlWon = runOneGame(ql, deck);

        deck.shuffle(static_cast<uint32_t>(i + 1)); // reset to same state
        bool mcWon = runOneGame(mc, deck);

        if (qlWon) { ++qlWins; }
        if (mcWon) { ++mcWins; }
    }

    ComparisonResult result;
    result.qlWinRate   = static_cast<double>(qlWins) / evalGames;
    result.mcWinRate   = static_cast<double>(mcWins) / evalGames;
    result.gamesPlayed = evalGames;

    // Print formatted report
    std::cout << "\n";
    std::cout << "\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\n";
    std::cout << " Agent comparison  ("
              << evalGames << " games, " << numDecks << " decks)\n";
    std::cout << "\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << " Q-Learning win rate:   " << result.qlWinRate * 100.0 << "%\n";
    std::cout << " Monte Carlo win rate:  " << result.mcWinRate * 100.0 << "%\n";

    double diff = (result.qlWinRate - result.mcWinRate) * 100.0;
    if (diff > 0.0) {
        std::cout << " Difference:            +" << diff << "% (Q-Learning leads)\n";
    } else if (diff < 0.0) {
        std::cout << " Difference:            " << diff << "% (Monte Carlo leads)\n";
    } else {
        std::cout << " Difference:            0.0% (tie)\n";
    }
    std::cout << "\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\n";
    std::cout << " Note: both agents use identical deck sequences.\n";
    std::cout << " A push is not counted as a win for either agent.\n";
    std::cout << "\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\n\n";

    return result;
}
