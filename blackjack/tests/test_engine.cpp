#include "doctest.h"
#include <stdexcept>
#include "../include/Card.h"
#include "../include/Dealer.h"
#include "../include/GameEngine.h"
#include "../include/Hand.h"
#include "../include/Player.h"
#include "../include/Types.h"

// ─── Player / bet / split / surrender ────────────────────────────────────────

TEST_CASE("Player - placeBet returns false with 0 chips") {
    Player p("Test", 0);
    CHECK(p.placeBet(MIN_BET) == false);
}

TEST_CASE("Player - placeBet returns false when chips < MIN_BET") {
    Player p("Test", MIN_BET - 1);
    CHECK(p.placeBet(MIN_BET) == false);
}

TEST_CASE("Player - placeBet succeeds and deducts chips") {
    Player p("Test", 100);
    CHECK(p.placeBet(50) == true);
    CHECK(p.getChips() == 50);
    CHECK(p.getCurrentBet() == 50);
}

TEST_CASE("Player - splitHand throws on non-pair") {
    Player p("Test", 500);
    p.getHand(0).addCard(Card(Suit::Spades, Rank::Ace));
    p.getHand(0).addCard(Card(Suit::Hearts, Rank::King));
    p.placeBet(50);
    CHECK_THROWS_AS(p.splitHand(), std::logic_error);
}

TEST_CASE("Player - surrender after markActed throws") {
    Player p("Test", 500);
    p.markActed();
    CHECK_THROWS_AS(p.surrender(), std::logic_error);
}

TEST_CASE("Player - surrender before acting succeeds") {
    Player p("Test", 500);
    CHECK_NOTHROW(p.surrender());
    CHECK(p.hasSurrendered() == true);
}

// ─── Dealer shouldHit ────────────────────────────────────────────────────────

TEST_CASE("Dealer - shouldHit true at hard 16") {
    Dealer d;
    d.getHand(0).addCard(Card(Suit::Spades, Rank::Ten));
    d.getHand(0).addCard(Card(Suit::Hearts, Rank::Six));
    CHECK(d.getHand(0).getValue() == 16);
    CHECK(d.shouldHit() == true);
}

TEST_CASE("Dealer - shouldHit false at hard 17") {
    Dealer d;
    d.getHand(0).addCard(Card(Suit::Spades, Rank::Ten));
    d.getHand(0).addCard(Card(Suit::Hearts, Rank::Seven));
    CHECK(d.getHand(0).getValue() == 17);
    CHECK(d.getHand(0).isSoft() == false);
    CHECK(d.shouldHit() == false);
}

TEST_CASE("Dealer - shouldHit true at soft 17") {
    Dealer d;
    d.getHand(0).addCard(Card(Suit::Spades, Rank::Ace));
    d.getHand(0).addCard(Card(Suit::Hearts, Rank::Six));
    CHECK(d.getHand(0).getValue() == 17);
    CHECK(d.getHand(0).isSoft() == true);
    CHECK(d.shouldHit() == true);
}

TEST_CASE("Dealer - shouldHit false at hard 18") {
    Dealer d;
    d.getHand(0).addCard(Card(Suit::Spades, Rank::Ten));
    d.getHand(0).addCard(Card(Suit::Hearts, Rank::Eight));
    CHECK(d.getHand(0).getValue() == 18);
    CHECK(d.shouldHit() == false);
}

// ─── calculatePayout ─────────────────────────────────────────────────────────

TEST_CASE("calculatePayout - natural blackjack vs non-BJ dealer") {
    GameEngine engine(1, 1, STARTING_CHIPS, 42, true);
    Hand player;
    player.addCard(Card(Suit::Spades, Rank::Ace));
    player.addCard(Card(Suit::Hearts, Rank::King));

    Hand dealer;
    dealer.addCard(Card(Suit::Diamonds, Rank::Ten));
    dealer.addCard(Card(Suit::Clubs, Rank::Seven));

    int bet    = 100;
    int payout = engine.calculatePayout(player, dealer, bet);
    CHECK(payout == bet + (bet * BLACKJACK_PAYOUT_NUM / BLACKJACK_PAYOUT_DEN));
}

TEST_CASE("calculatePayout - push returns original bet") {
    GameEngine engine(1, 1, STARTING_CHIPS, 42, true);
    Hand player;
    player.addCard(Card(Suit::Spades, Rank::Ten));
    player.addCard(Card(Suit::Hearts, Rank::Seven));

    Hand dealer;
    dealer.addCard(Card(Suit::Diamonds, Rank::Ten));
    dealer.addCard(Card(Suit::Clubs, Rank::Seven));

    int bet = 100;
    CHECK(engine.calculatePayout(player, dealer, bet) == bet);
}

TEST_CASE("calculatePayout - player bust is not passed (bet*2 when dealer busts)") {
    GameEngine engine(1, 1, STARTING_CHIPS, 42, true);
    Hand player;
    player.addCard(Card(Suit::Spades, Rank::Ten));
    player.addCard(Card(Suit::Hearts, Rank::Eight));

    Hand dealer;
    dealer.addCard(Card(Suit::Diamonds, Rank::Ten));
    dealer.addCard(Card(Suit::Clubs, Rank::Eight));
    dealer.addCard(Card(Suit::Spades, Rank::Five));  // dealer busts

    CHECK(dealer.isBust() == true);
    int bet = 100;
    CHECK(engine.calculatePayout(player, dealer, bet) == bet * 2);
}

// ─── Smoke test ───────────────────────────────────────────────────────────────

TEST_CASE("GameEngine - full round smoke test with seed 42 completes without exception") {
    GameEngine engine(1, 1, STARTING_CHIPS, 42, /*aiOnly=*/true);
    CHECK_NOTHROW(engine.run());
}
