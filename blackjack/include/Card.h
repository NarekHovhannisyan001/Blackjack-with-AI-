#pragma once

#include "Types.h"
#include <string>

/// Represents a single immutable playing card with a suit and rank.
/// Card is a value type — it is fully copyable and contains no heap allocations.
class Card {
public:
    /// Constructs a card with the given suit and rank.
    /// @param suit The suit of the card.
    /// @param rank The rank of the card.
    Card(Suit suit, Rank rank);

    /// Returns the suit of this card.
    Suit getSuit() const;

    /// Returns the rank of this card.
    Rank getRank() const;

    /// Returns the blackjack point value of this card.
    /// Two–Ten return their face value. Jack, Queen, King return 10.
    /// Ace returns 11; Hand::getValue() is responsible for reducing it to 1.
    int getValue() const;

    /// Returns the full name of this card, e.g. "Ace of Spades", "10 of Hearts".
    std::string toString() const;

    /// Returns the short display string used in console card rendering,
    /// e.g. "A♠", "10♥", "K♦", "2♣".
    std::string toShortString() const;

private:
    Suit m_suit;
    Rank m_rank;
};
