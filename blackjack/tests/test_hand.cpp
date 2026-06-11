#include "doctest.h"
#include "../include/Hand.h"

// Helper to build a hand from a list of (Suit, Rank) pairs
static Hand makeHand(std::initializer_list<std::pair<Suit, Rank>> cards,
                     bool fromSplit = false) {
    Hand h(fromSplit);
    for (const auto& p : cards) {
        h.addCard(Card(p.first, p.second));
    }
    return h;
}

TEST_CASE("Hand::getValue - [A] == 11") {
    Hand h = makeHand({{Suit::Spades, Rank::Ace}});
    CHECK(h.getValue() == 11);
}

TEST_CASE("Hand::isSoft - [A] is soft") {
    Hand h = makeHand({{Suit::Spades, Rank::Ace}});
    CHECK(h.isSoft() == true);
}

TEST_CASE("Hand::getValue - [A, 5] == 16, isSoft") {
    Hand h = makeHand({{Suit::Spades, Rank::Ace}, {Suit::Hearts, Rank::Five}});
    CHECK(h.getValue() == 16);
    CHECK(h.isSoft() == true);
}

TEST_CASE("Hand::getValue - [A, K] == 21, isBlackjack, not soft") {
    Hand h = makeHand({{Suit::Spades, Rank::Ace}, {Suit::Hearts, Rank::King}});
    CHECK(h.getValue() == 21);
    CHECK(h.isBlackjack() == true);
    CHECK(h.isSoft() == false);
}

TEST_CASE("Hand::isBlackjack - [A, K, 2] is not blackjack (3 cards)") {
    Hand h = makeHand({
        {Suit::Spades,   Rank::Ace},
        {Suit::Hearts,   Rank::King},
        {Suit::Diamonds, Rank::Two}
    });
    CHECK(h.getValue() == 13);
    CHECK(h.isBlackjack() == false);
}

TEST_CASE("Hand::getValue - [A, A] == 12, isSoft") {
    Hand h = makeHand({
        {Suit::Spades, Rank::Ace},
        {Suit::Hearts, Rank::Ace}
    });
    CHECK(h.getValue() == 12);
    CHECK(h.isSoft() == true);
}

TEST_CASE("Hand::getValue - [A, A, 9] == 21, not soft") {
    Hand h = makeHand({
        {Suit::Spades,   Rank::Ace},
        {Suit::Hearts,   Rank::Ace},
        {Suit::Diamonds, Rank::Nine}
    });
    CHECK(h.getValue() == 21);
    CHECK(h.isSoft() == false);
}

TEST_CASE("Hand::getValue - [A, A, A] == 13") {
    Hand h = makeHand({
        {Suit::Spades,   Rank::Ace},
        {Suit::Hearts,   Rank::Ace},
        {Suit::Diamonds, Rank::Ace}
    });
    CHECK(h.getValue() == 13);
}

TEST_CASE("Hand::getValue - [A, A, A, A] == 14") {
    Hand h = makeHand({
        {Suit::Spades,   Rank::Ace},
        {Suit::Hearts,   Rank::Ace},
        {Suit::Diamonds, Rank::Ace},
        {Suit::Clubs,    Rank::Ace}
    });
    CHECK(h.getValue() == 14);
}

TEST_CASE("Hand::isBust - [10, 10, 2] is bust") {
    Hand h = makeHand({
        {Suit::Spades,   Rank::Ten},
        {Suit::Hearts,   Rank::Ten},
        {Suit::Diamonds, Rank::Two}
    });
    CHECK(h.getValue() == 22);
    CHECK(h.isBust() == true);
}

TEST_CASE("Hand::isPair - [10, 10] is a pair") {
    Hand h = makeHand({
        {Suit::Spades, Rank::Ten},
        {Suit::Hearts, Rank::Ten}
    });
    CHECK(h.isPair() == true);
}

TEST_CASE("Hand::isPair - [10, J] is NOT a pair (different rank)") {
    Hand h = makeHand({
        {Suit::Spades, Rank::Ten},
        {Suit::Hearts, Rank::Jack}
    });
    CHECK(h.isPair() == false);
}

TEST_CASE("Hand::isBlackjack - fromSplit hand [A, K] is not blackjack") {
    Hand h = makeHand(
        {{Suit::Spades, Rank::Ace}, {Suit::Hearts, Rank::King}},
        /*fromSplit=*/true
    );
    CHECK(h.getValue() == 21);
    CHECK(h.isBlackjack() == false);
}

TEST_CASE("Hand::clear - empty hand has value 0 and cardCount 0") {
    Hand h = makeHand({{Suit::Spades, Rank::Ace}, {Suit::Hearts, Rank::King}});
    h.clear();
    CHECK(h.getValue() == 0);
    CHECK(h.cardCount() == 0);
}
