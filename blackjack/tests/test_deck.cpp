#include "doctest.h"
#include "../include/Deck.h"
#include <stdexcept>

TEST_CASE("Deck - single deck has 52 cards") {
    Deck d(1);
    CHECK(d.cardsRemaining() == 52);
}

TEST_CASE("Deck - six-deck shoe has 312 cards") {
    Deck d(6);
    CHECK(d.cardsRemaining() == 312);
}

TEST_CASE("Deck - deal() reduces cardsRemaining by 1 each call") {
    Deck d(1);
    int initial = d.cardsRemaining();
    d.deal();
    CHECK(d.cardsRemaining() == initial - 1);
    d.deal();
    CHECK(d.cardsRemaining() == initial - 2);
}

TEST_CASE("Deck - deal() throws std::logic_error on empty shoe") {
    Deck d(1);
    for (int i = 0; i < 52; ++i) {
        d.deal();
    }
    CHECK_THROWS_AS(d.deal(), std::logic_error);
}

TEST_CASE("Deck - seed 42 always deals same first 5 cards") {
    Deck d1(1);
    d1.shuffle(42);
    Deck d2(1);
    d2.shuffle(42);
    for (int i = 0; i < 5; ++i) {
        Card c1 = d1.deal();
        Card c2 = d2.deal();
        CHECK(c1.getRank() == c2.getRank());
        CHECK(c1.getSuit() == c2.getSuit());
    }
}

TEST_CASE("Deck - needsReshuffle false with 60 cards remaining") {
    Deck d(2);
    d.shuffle(42);
    for (int i = 0; i < 44; ++i) {
        d.deal();
    }
    CHECK(d.cardsRemaining() == 60);
    CHECK(d.needsReshuffle() == false);
}

TEST_CASE("Deck - needsReshuffle true with 51 cards remaining") {
    Deck d(2);
    d.shuffle(42);
    for (int i = 0; i < 53; ++i) {
        d.deal();
    }
    CHECK(d.cardsRemaining() == 51);
    CHECK(d.needsReshuffle() == true);
}
