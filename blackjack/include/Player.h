#pragma once

#include "Hand.h"
#include "Types.h"
#include <string>
#include <vector>

/// Represents a human or AI player at the table.
/// Owns one or two Hand objects (two after a split).
/// All chip accounting (bets, winnings, insurance) is handled here.
class Player {
public:
    /// Constructs a player with the given name, starting chip count, and AI flag.
    Player(const std::string& name, int startingChips, bool isAI = false);

    virtual ~Player() = default;
    Player(const Player&) = default;
    Player& operator=(const Player&) = default;

    /// Returns the player's display name.
    const std::string& getName() const;

    /// Returns the player's current chip count.
    int getChips() const;

    /// Returns the chip amount the player wagered this round.
    int getCurrentBet() const;

    /// Returns true if this player is controlled by AI logic.
    bool isAI() const;

    /// Returns true if the player has surrendered this round.
    bool hasSurrendered() const;

    /// Returns true if the player has taken at least one action this turn.
    bool hasActed() const;

    /// Returns the insurance bet placed this round (0 if none).
    int getInsuranceBet() const;

    /// Returns a mutable reference to the hand at the given index.
    /// @param index Hand index (0 = main, 1 = split hand).
    /// @throws std::logic_error if index is out of range.
    Hand& getHand(int index = 0);

    /// Returns a const reference to the hand at the given index.
    /// @param index Hand index (0 = main, 1 = split hand).
    /// @throws std::logic_error if index is out of range.
    const Hand& getHand(int index = 0) const;

    /// Returns the number of active hands (1 normally, 2 after a split).
    int handCount() const;

    /// Places a bet of the given amount if it is within [MIN_BET, m_chips].
    /// Deducts the amount from m_chips on success.
    /// @return true on success, false if amount is out of range.
    bool placeBet(int amount);

    /// Places an insurance side-bet. Amount must be > 0 and <= m_chips / 2.
    /// Deducts the amount from m_chips on success.
    /// @return true on success, false if amount is invalid.
    bool placeInsuranceBet(int amount);

    /// Adds amount to m_chips. Used for wins, pushes, and surrender half-returns.
    void receiveWinnings(int amount);

    /// Splits m_hands[0] into two single-card hands when the first hand is a pair.
    /// Deducts m_currentBet for the second hand's wager.
    /// @throws std::logic_error if the hand is not a pair.
    /// @throws std::logic_error if m_chips < m_currentBet.
    void splitHand();

    /// Records that the player has taken their first action this turn.
    void markActed();

    /// Marks the player as surrendered. Must be called before markActed().
    /// @throws std::logic_error if markActed() was already called this turn.
    void surrender();

    /// Resets all per-round state: clears hands, bets, flags.
    /// Leaves m_chips unchanged and creates one fresh empty Hand.
    void resetForNewRound();

private:
    std::string       m_name;
    int               m_chips;
    int               m_currentBet;
    std::vector<Hand> m_hands;
    bool              m_isAI;
    bool              m_hasSurrendered;
    int               m_insuranceBet;
    bool              m_hasActed;
};
