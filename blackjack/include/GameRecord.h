#pragma once

#include <string>

class Hand;

/// Plain data transfer object holding all fields for one recorded round.
/// All members are public — this is a DTO, not a domain object.
struct GameRecord {
    int         sessionId         = 0;
    int         roundId           = 0;
    std::string timestamp;
    std::string playerHand;       // e.g. "A♠ K♥" or "A♠ K♥ | 5♦ 9♣" for split
    std::string dealerHand;       // e.g. "K♦ 7♣" or "K♦ ?" when hole hidden
    std::string actionTaken;      // "Hit", "Stand", "Double", "Split", "Surrender"
    std::string outcome;          // "Win", "Lose", "Push", "Blackjack", "Surrender"
    int         chipsBefore       = 0;
    int         chipsAfter        = 0;
    int         betAmount         = 0;
    bool        isSoftHand        = false;
    int         dealerUpcardValue = 0;
};

/// Serialises a Hand to a space-separated short-string card list, e.g. "A♠ K♥".
std::string serializeHand(const Hand& hand);

/// Returns the current local time as an ISO 8601 string, e.g. "2024-03-15T14:32:01".
std::string currentTimestamp();
