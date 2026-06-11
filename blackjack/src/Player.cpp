#include "Player.h"
#include <stdexcept>

Player::Player(const std::string& name, int startingChips, bool isAI)
    : m_name(name), m_chips(startingChips), m_currentBet(0),
      m_isAI(isAI), m_hasSurrendered(false), m_insuranceBet(0), m_hasActed(false) {
    m_hands.emplace_back();
}

const std::string& Player::getName() const { return m_name; }
int  Player::getChips()        const { return m_chips; }
int  Player::getCurrentBet()   const { return m_currentBet; }
bool Player::isAI()            const { return m_isAI; }
bool Player::hasSurrendered()  const { return m_hasSurrendered; }
bool Player::hasActed()        const { return m_hasActed; }
int  Player::getInsuranceBet() const { return m_insuranceBet; }

Hand& Player::getHand(int index) {
    if (index < 0 || index >= static_cast<int>(m_hands.size())) {
        throw std::logic_error("Hand index out of range");
    }
    return m_hands[static_cast<size_t>(index)];
}

const Hand& Player::getHand(int index) const {
    if (index < 0 || index >= static_cast<int>(m_hands.size())) {
        throw std::logic_error("Hand index out of range");
    }
    return m_hands[static_cast<size_t>(index)];
}

int Player::handCount() const {
    return static_cast<int>(m_hands.size());
}

bool Player::placeBet(int amount) {
    if (amount < MIN_BET || amount > m_chips) {
        return false;
    }
    m_chips -= amount;
    m_currentBet = amount;
    return true;
}

bool Player::placeInsuranceBet(int amount) {
    if (amount <= 0 || amount > m_chips / 2) {
        return false;
    }
    m_chips -= amount;
    m_insuranceBet = amount;
    return true;
}

void Player::receiveWinnings(int amount) {
    m_chips += amount;
}

void Player::splitHand() {
    if (!m_hands[0].isPair()) {
        throw std::logic_error("splitHand() called on a non-pair hand");
    }
    if (m_chips < m_currentBet) {
        throw std::logic_error("Insufficient chips to split");
    }

    Card firstCard  = m_hands[0].getCards()[0];
    Card secondCard = m_hands[0].getCards()[1];

    m_hands[0].clear();
    m_hands[0].addCard(firstCard);

    m_chips -= m_currentBet;

    m_hands.emplace_back(true);
    m_hands[1].addCard(secondCard);
}

void Player::markActed() {
    m_hasActed = true;
}

void Player::surrender() {
    if (m_hasActed) {
        throw std::logic_error("surrender() called after player has already acted");
    }
    m_hasSurrendered = true;
}

void Player::resetForNewRound() {
    m_hands.clear();
    m_hands.emplace_back();
    m_currentBet    = 0;
    m_insuranceBet  = 0;
    m_hasSurrendered = false;
    m_hasActed       = false;
}
