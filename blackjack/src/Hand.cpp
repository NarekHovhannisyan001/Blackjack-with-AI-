#include "Hand.h"

Hand::Hand()
    : m_fromSplit(false) {}

Hand::Hand(bool fromSplit)
    : m_fromSplit(fromSplit) {}

void Hand::addCard(const Card& card) {
    m_cards.push_back(card);
}

int Hand::getValue() const {
    int aceCount = 0;
    int total = 0;
    for (const Card& c : m_cards) {
        if (c.getRank() == Rank::Ace) {
            aceCount++;
            total += 11;
        } else {
            total += c.getValue();
        }
    }
    while (total > 21 && aceCount > 0) {
        total -= 10;
        aceCount--;
    }
    return total;
}

bool Hand::isBust() const {
    return getValue() > 21;
}

bool Hand::isBlackjack() const {
    return m_cards.size() == 2 && getValue() == 21 && !m_fromSplit;
}

bool Hand::isSoft() const {
    int aceCount = 0;
    int total = 0;
    for (const Card& c : m_cards) {
        if (c.getRank() == Rank::Ace) {
            aceCount++;
            total += 11;
        } else {
            total += c.getValue();
        }
    }
    while (total > 21 && aceCount > 0) {
        total -= 10;
        aceCount--;
    }
    return (total < 21) && (aceCount > 0);
}

bool Hand::isPair() const {
    return m_cards.size() == 2 && m_cards[0].getRank() == m_cards[1].getRank();
}

bool Hand::isFromSplit() const {
    return m_fromSplit;
}

int Hand::cardCount() const {
    return static_cast<int>(m_cards.size());
}

const std::vector<Card>& Hand::getCards() const {
    return m_cards;
}

void Hand::clear() {
    m_cards.clear();
}
