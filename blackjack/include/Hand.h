#pragma once

#include "Card.h"
#include <vector>

/// Represents a player's or dealer's hand of playing cards.
/// Handles Ace reduction correctly for multiple Aces.
/// A hand created by splitting cannot be awarded a natural blackjack.
class Hand {
public:
    /// Constructs an empty hand that was not created from a split.
    Hand();

    /// Constructs an empty hand and marks whether it came from a split.
    /// @param fromSplit True if this hand was created by player.splitHand().
    explicit Hand(bool fromSplit);

    /// Adds a card to the hand.
    /// @param card The card to add.
    void addCard(const Card& card);

    /// Returns the best blackjack value of this hand.
    /// Aces initially count as 11; each is reduced to 1 (subtract 10) while
    /// the total exceeds 21. Handles any number of Aces correctly.
    int getValue() const;

    /// Returns true if getValue() exceeds 21.
    bool isBust() const;

    /// Returns true if the hand has exactly 2 cards totalling 21 and was
    /// not created from a split. Split 21 is not a natural blackjack.
    bool isBlackjack() const;

    /// Returns true if the hand has at least one Ace currently contributing 11
    /// and the total is strictly less than 21. A total of exactly 21 is never
    /// considered soft because the dealer always stands and no reduction matters.
    bool isSoft() const;

    /// Returns true if the hand has exactly 2 cards of the same Rank.
    /// Note: same point value is not sufficient — [10, J] is not a pair.
    bool isPair() const;

    /// Returns true if this hand was created by a split.
    bool isFromSplit() const;

    /// Returns the number of cards in the hand.
    int cardCount() const;

    /// Returns a const reference to the underlying card collection.
    const std::vector<Card>& getCards() const;

    /// Removes all cards from the hand.
    void clear();

private:
    std::vector<Card> m_cards;
    bool              m_fromSplit;
};
