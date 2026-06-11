#pragma once

#include "Card.h"
#include "Types.h"
#include <vector>
#include <cstdint>

/// Represents a shoe containing one or more standard 52-card decks.
/// Deck is owned exclusively by GameEngine as a value member and must never be copied.
/// All other classes receive a Deck& reference for the duration of a single call only.
class Deck {
public:
    /// Constructs a shoe with the given number of decks, builds the card set,
    /// and shuffles it with a random (time-based) seed.
    /// @param numDecks Number of standard 52-card decks to include (default 1).
    explicit Deck(int numDecks = 1);

    /// Shuffles the shoe.
    /// If seed == 0, uses the current time as the seed (non-deterministic).
    /// If seed != 0, rebuilds the shoe from scratch first then shuffles with that
    /// exact seed, guaranteeing a fully deterministic result for testing.
    /// @param seed Shuffle seed; 0 means random.
    void shuffle(uint32_t seed = 0);

    /// Removes and returns the top card of the shoe.
    /// @throws std::logic_error if the shoe is empty.
    Card deal();

    /// Returns the number of cards currently remaining in the shoe.
    int cardsRemaining() const;

    /// Returns true when cardsRemaining() is less than the cut position,
    /// indicating GameEngine should call reshuffle() before the next round.
    bool needsReshuffle() const;

    /// Rebuilds the shoe from scratch and performs a fresh random shuffle.
    void reshuffle();

private:
    std::vector<Card> m_shoe;
    int               m_numDecks;
    int               m_cutPosition;

    /// Clears m_shoe and refills it with m_numDecks * 52 cards in suit/rank order.
    void buildShoe();
};
