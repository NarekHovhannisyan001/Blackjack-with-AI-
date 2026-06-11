#include "StateEncoder.h"
#include <stdexcept>
#include <string>

void StateEncoder::validateState(const GameStateSnapshot& state) const {
    if (state.playerTotal < 4 || state.playerTotal > 21) {
        throw std::logic_error(
            "StateEncoder: invalid playerTotal " +
            std::to_string(state.playerTotal) + " (must be 4-21)");
    }
    if (state.dealerUpcardValue < 2 || state.dealerUpcardValue > 11) {
        throw std::logic_error(
            "StateEncoder: invalid dealerUpcardValue " +
            std::to_string(state.dealerUpcardValue) + " (must be 2-11)");
    }
}

std::string StateEncoder::countBucket(int runningCount) const {
    if (runningCount <= -4) { return "-2"; }
    if (runningCount <= -1) { return "-1"; }
    if (runningCount == 0)  { return "0";  }
    if (runningCount <= 3)  { return "+1"; }
    return "+2";
}

std::string StateEncoder::encode(const GameStateSnapshot& state) const {
    validateState(state);
    // Reserve space: max "21|1|11|+2" = 10 chars
    std::string key;
    key.reserve(12);
    key += std::to_string(state.playerTotal);
    key += '|';
    key += (state.isSoftHand ? '1' : '0');
    key += '|';
    key += std::to_string(state.dealerUpcardValue);
    key += '|';
    key += countBucket(state.runningCount);
    return key;
}

std::string StateEncoder::encodeSimple(const GameStateSnapshot& state) const {
    validateState(state);
    std::string key;
    key.reserve(8);
    key += std::to_string(state.playerTotal);
    key += '|';
    key += (state.isSoftHand ? '1' : '0');
    key += '|';
    key += std::to_string(state.dealerUpcardValue);
    return key;
}

int StateEncoder::stateSpaceSize() {
    // 18 totals (4-21) × 2 soft × 10 upcards (2-11) × 5 count buckets
    return 18 * 2 * 10 * 5;
}
