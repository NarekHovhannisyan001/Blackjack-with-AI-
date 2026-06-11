#include "doctest.h"
#include "../include/Card.h"

TEST_CASE("Card::getValue - Ace returns 11") {
    Card c(Suit::Spades, Rank::Ace);
    CHECK(c.getValue() == 11);
}

TEST_CASE("Card::getValue - face cards return 10") {
    CHECK(Card(Suit::Hearts,   Rank::King).getValue()  == 10);
    CHECK(Card(Suit::Diamonds, Rank::Queen).getValue() == 10);
    CHECK(Card(Suit::Clubs,    Rank::Jack).getValue()  == 10);
}

TEST_CASE("Card::getValue - numbered cards return face value") {
    CHECK(Card(Suit::Spades,   Rank::Two).getValue()   == 2);
    CHECK(Card(Suit::Hearts,   Rank::Three).getValue() == 3);
    CHECK(Card(Suit::Diamonds, Rank::Four).getValue()  == 4);
    CHECK(Card(Suit::Clubs,    Rank::Five).getValue()  == 5);
    CHECK(Card(Suit::Spades,   Rank::Six).getValue()   == 6);
    CHECK(Card(Suit::Hearts,   Rank::Seven).getValue() == 7);
    CHECK(Card(Suit::Diamonds, Rank::Eight).getValue() == 8);
    CHECK(Card(Suit::Clubs,    Rank::Nine).getValue()  == 9);
    CHECK(Card(Suit::Spades,   Rank::Ten).getValue()   == 10);
}

TEST_CASE("Card::toString - Ace of Spades") {
    Card c(Suit::Spades, Rank::Ace);
    CHECK(c.toString() == "Ace of Spades");
}

TEST_CASE("Card::toString - 10 of Hearts") {
    Card c(Suit::Hearts, Rank::Ten);
    CHECK(c.toString() == "10 of Hearts");
}

TEST_CASE("Card::toShortString - A♠") {
    Card c(Suit::Spades, Rank::Ace);
    CHECK(c.toShortString() == "A\xe2\x99\xa0");
}

TEST_CASE("Card::toShortString - 10♥") {
    Card c(Suit::Hearts, Rank::Ten);
    CHECK(c.toShortString() == "10\xe2\x99\xa5");
}
