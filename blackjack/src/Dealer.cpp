#include "Dealer.h"
#include "Deck.h"

Dealer::Dealer()
    : Player("Dealer", 0), m_holeCardHidden(false) {}

void Dealer::hideHoleCard()   { m_holeCardHidden = true; }
void Dealer::revealHoleCard() { m_holeCardHidden = false; }
bool Dealer::isHoleCardHidden() const { return m_holeCardHidden; }

bool Dealer::shouldHit() const {
    int val = getHand(0).getValue();
    if (val <= 16) {
        return true;
    }
    if (getHand(0).isSoft() && val == 17) {
        return true;
    }
    return false;
}

void Dealer::playTurn(Deck& deck) {
    revealHoleCard();
    while (shouldHit()) {
        getHand(0).addCard(deck.deal());
    }
}
