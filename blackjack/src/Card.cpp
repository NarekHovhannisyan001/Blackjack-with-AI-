#include "Card.h"

Card::Card(Suit suit, Rank rank)
    : m_suit(suit), m_rank(rank) {}

Suit Card::getSuit() const {
    return m_suit;
}

Rank Card::getRank() const {
    return m_rank;
}

int Card::getValue() const {
    switch (m_rank) {
        case Rank::Two:   return 2;
        case Rank::Three: return 3;
        case Rank::Four:  return 4;
        case Rank::Five:  return 5;
        case Rank::Six:   return 6;
        case Rank::Seven: return 7;
        case Rank::Eight: return 8;
        case Rank::Nine:  return 9;
        case Rank::Ten:   return 10;
        case Rank::Jack:  return 10;
        case Rank::Queen: return 10;
        case Rank::King:  return 10;
        case Rank::Ace:   return 11;
    }
    return 0;
}

std::string Card::toString() const {
    std::string rankStr;
    switch (m_rank) {
        case Rank::Two:   rankStr = "Two";   break;
        case Rank::Three: rankStr = "Three"; break;
        case Rank::Four:  rankStr = "Four";  break;
        case Rank::Five:  rankStr = "Five";  break;
        case Rank::Six:   rankStr = "Six";   break;
        case Rank::Seven: rankStr = "Seven"; break;
        case Rank::Eight: rankStr = "Eight"; break;
        case Rank::Nine:  rankStr = "Nine";  break;
        case Rank::Ten:   rankStr = "10";    break;
        case Rank::Jack:  rankStr = "Jack";  break;
        case Rank::Queen: rankStr = "Queen"; break;
        case Rank::King:  rankStr = "King";  break;
        case Rank::Ace:   rankStr = "Ace";   break;
    }

    std::string suitStr;
    switch (m_suit) {
        case Suit::Hearts:   suitStr = "Hearts";   break;
        case Suit::Diamonds: suitStr = "Diamonds"; break;
        case Suit::Clubs:    suitStr = "Clubs";    break;
        case Suit::Spades:   suitStr = "Spades";   break;
    }

    return rankStr + " of " + suitStr;
}

std::string Card::toShortString() const {
    std::string rankStr;
    switch (m_rank) {
        case Rank::Two:   rankStr = "2";  break;
        case Rank::Three: rankStr = "3";  break;
        case Rank::Four:  rankStr = "4";  break;
        case Rank::Five:  rankStr = "5";  break;
        case Rank::Six:   rankStr = "6";  break;
        case Rank::Seven: rankStr = "7";  break;
        case Rank::Eight: rankStr = "8";  break;
        case Rank::Nine:  rankStr = "9";  break;
        case Rank::Ten:   rankStr = "10"; break;
        case Rank::Jack:  rankStr = "J";  break;
        case Rank::Queen: rankStr = "Q";  break;
        case Rank::King:  rankStr = "K";  break;
        case Rank::Ace:   rankStr = "A";  break;
    }

    std::string suitStr;
    switch (m_suit) {
        case Suit::Hearts:   suitStr = "\xe2\x99\xa5"; break;  // ♥
        case Suit::Diamonds: suitStr = "\xe2\x99\xa6"; break;  // ♦
        case Suit::Clubs:    suitStr = "\xe2\x99\xa3"; break;  // ♣
        case Suit::Spades:   suitStr = "\xe2\x99\xa0"; break;  // ♠
    }

    return rankStr + suitStr;
}
