#pragma once

#include "ConsoleView.h"
#include "Dealer.h"
#include "Deck.h"
#include "Player.h"
#include "Types.h"
#include <cstdint>
#include <string>
#include <vector>

/// Top-level game controller. Owns the deck, dealer, players, and view.
/// Drives the game through a finite-state machine until GameOver.
/// When all players are AI (aiOnly == true) the game runs one round
/// non-interactively — used by automated tests.
class GameEngine {
public:
    /// Constructs the engine with the given number of players and decks.
    /// @param numPlayers   Number of human players (1–MAX_PLAYERS).
    /// @param numDecks     Number of decks in the shoe.
    /// @param startingChips Chips each player starts with.
    /// @param deckSeed     If non-zero, shuffles the shoe deterministically.
    /// @param aiOnly       If true, all players are AI and no prompts are shown.
    GameEngine(int numPlayers, int numDecks, int startingChips,
               uint32_t deckSeed = 0, bool aiOnly = false);

    /// Runs the game loop until GameState::GameOver is reached.
    void run();

    /// Computes and returns the chips returned to the player for one hand.
    /// Returns 0 for a loss; returns original bet for a push; returns more for wins.
    /// Integer arithmetic truncates odd bets on the 3:2 blackjack payout —
    /// this matches standard casino behaviour.
    /// @param playerHand The settled player hand (must not be bust).
    /// @param dealerHand The dealer's final hand.
    /// @param bet        The original wager for this hand.
    int calculatePayout(const Hand& playerHand,
                        const Hand& dealerHand, int bet) const;

private:
    Deck                m_deck;
    Dealer              m_dealer;
    std::vector<Player> m_players;
    GameState           m_state;
    ConsoleView         m_view;
    int                 m_roundNumber;

    /// Validates and performs a state transition.
    /// @throws std::logic_error on an illegal transition.
    void transitionTo(GameState next);

    /// Dispatches to the execute method matching m_state.
    void executeCurrentState();

    void executeBetting();
    void executeDealing();
    void executeInsurance();
    void executePlayerTurns();
    void executeDealerTurn();
    void executeSettling();

    /// Runs one player hand to completion (hit/stand/double/split/surrender).
    void runSingleHandTurn(Player& player, int handIndex);

    static bool        isValidTransition(GameState from, GameState to);
    static std::string stateToString(GameState s);
};
