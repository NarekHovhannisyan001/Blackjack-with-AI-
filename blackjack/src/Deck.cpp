#include "Deck.h"
#include <algorithm>
#include <chrono>
#include <random>
#include <stdexcept>

Deck::Deck(int numDecks)
    : m_numDecks(numDecks), m_cutPosition(RESHUFFLE_THRESHOLD) {
    buildShoe();
    shuffle();
}

void Deck::buildShoe() {
    m_shoe.clear();
    const Suit suits[] = { Suit::Hearts, Suit::Diamonds, Suit::Clubs, Suit::Spades };
    const Rank ranks[] = {
        Rank::Two,  Rank::Three, Rank::Four, Rank::Five, Rank::Six,
        Rank::Seven, Rank::Eight, Rank::Nine, Rank::Ten,
        Rank::Jack, Rank::Queen, Rank::King, Rank::Ace
    };
    for (int d = 0; d < m_numDecks; ++d) {
        for (const Suit& s : suits) {
            for (const Rank& r : ranks) {
                m_shoe.push_back(Card(s, r));
            }
        }
    }
}

void Deck::shuffle(uint32_t seed) {
    uint32_t actualSeed;
    if (seed != 0) {
        buildShoe();
        actualSeed = seed;
    } else {
        actualSeed = static_cast<uint32_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );
    }
    std::mt19937 rng(actualSeed);
    std::shuffle(m_shoe.begin(), m_shoe.end(), rng);
}

Card Deck::deal() {
    if (m_shoe.empty()) {
        throw std::logic_error("Cannot deal from an empty shoe");
    }
    Card c = m_shoe.front();
    m_shoe.erase(m_shoe.begin());
    return c;
}

int Deck::cardsRemaining() const {
    return static_cast<int>(m_shoe.size());
}

bool Deck::needsReshuffle() const {
    return cardsRemaining() < m_cutPosition;
}

void Deck::reshuffle() {
    buildShoe();
    shuffle();
}
