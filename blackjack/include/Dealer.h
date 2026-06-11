#pragma once

#include "Player.h"

class Deck;

/// Represents the dealer. Inherits Player for hand and chip tracking.
/// Enforces casino hit-on-soft-17 rules and manages the hidden hole card.
class Dealer : public Player {
public:
    /// Constructs the dealer with no chips and hole card visible.
    Dealer();

    /// Hides the hole card (second card) for display purposes.
    void hideHoleCard();

    /// Reveals the hole card so ConsoleView shows it.
    void revealHoleCard();

    /// Returns true if the hole card is currently hidden.
    bool isHoleCardHidden() const;

    /// Returns true if the dealer must take another card.
    /// Hits on hard 16 or below, hits on soft 17, stands on hard 17 or above.
    bool shouldHit() const;

    /// Reveals the hole card then draws cards until shouldHit() is false.
    /// @param deck The shoe to draw from (passed by reference for this call only).
    /// @throws std::logic_error if the shoe runs empty mid-turn.
    void playTurn(Deck& deck);

private:
    bool m_holeCardHidden;
};
