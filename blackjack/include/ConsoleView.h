#pragma once

#include "Dealer.h"
#include "Player.h"
#include "Types.h"
#include <string>
#include <vector>

/// Handles all console output and input for the game.
/// This is the ONLY class permitted to write to std::cout.
/// GameEngine may write to std::cerr for FSM debug logging only.
class ConsoleView {
public:
    ConsoleView() = default;

    /// Prints the welcome banner.
    void showWelcome();

    /// Prints the round header, e.g. "====== ROUND 3 ======".
    /// @param roundNumber The current round number.
    void showRoundStart(int roundNumber);

    /// Prints the full table state: dealer hand then each player's hands.
    /// @param players  All active players.
    /// @param dealer   The dealer.
    /// @param hideHole If true, the dealer's hole card is shown as [?].
    void showTable(const std::vector<Player>& players,
                   const Dealer& dealer, bool hideHole);

    /// Prints one player hand with value and optional annotations.
    /// @param player    The owning player.
    /// @param handIndex Which hand to show (0 = main, 1 = split).
    void showPlayerHand(const Player& player, int handIndex);

    /// Prints the dealer's hand, optionally hiding the hole card.
    /// @param dealer   The dealer.
    /// @param hideHole If true, second card is shown as [?].
    void showDealerHand(const Dealer& dealer, bool hideHole);

    /// Prints a bust notification for the given name.
    void showBustMessage(const std::string& name);

    /// Prints a blackjack notification for the given name.
    void showBlackjackMessage(const std::string& name);

    /// Prints a surrender notification for the given name.
    void showSurrenderMessage(const std::string& name);

    /// Prints a settlement result line.
    /// @param name     Player's name.
    /// @param result   The outcome type.
    /// @param amount   Bet / profit / loss amount relevant to this result.
    /// @param newTotal Player's chip total after this payout.
    void showPayoutResult(const std::string& name,
                          PayoutResult result, int amount, int newTotal);

    /// Prints the insurance offer prompt text.
    void showInsuranceOffer();

    /// Prints the game-over summary for a player.
    void showGameOver(const std::string& name, int finalChips);

    /// Prints the round separator line.
    void showRoundSeparator();

    /// Prompts the player for an action, showing only legal options.
    /// Loops until a valid character is entered.
    /// Accepts h/H s/S d/D p/P r/R.
    /// @param player    The acting player.
    /// @param handIndex Which hand the action applies to.
    PlayerAction promptPlayerAction(const Player& player, int handIndex);

    /// Prompts the player for a bet between MIN_BET and their chip count.
    /// Loops until a valid bet is entered.
    /// @param player The betting player.
    /// @return A valid bet amount.
    int promptBet(const Player& player);

    /// Prompts the player to accept or decline insurance (y/Y or n/N).
    /// Loops until a valid character is entered.
    bool promptInsurance(const Player& player);

    /// Prompts "play again?" and returns true for y/Y, false for n/N.
    /// Loops until a valid character is entered.
    bool promptPlayAgain();

private:
    /// Builds the card display string for a hand, e.g. "[A♠] [K♥]".
    /// If hideSecondCard is true, the last card is replaced with [?].
    std::string formatHand(const Hand& hand, bool hideSecondCard = false) const;
};
