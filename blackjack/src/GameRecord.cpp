#include "GameRecord.h"
#include "Hand.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string serializeHand(const Hand& hand) {
    const auto& cards = hand.getCards();
    std::string result;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) { result += " "; }
        result += cards[i].toShortString();
    }
    return result;
}

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}
