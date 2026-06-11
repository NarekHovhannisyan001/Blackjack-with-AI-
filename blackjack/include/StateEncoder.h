#pragma once

#include "Types.h"
#include <string>

/// Converts a GameStateSnapshot into a collision-free string key for use
/// as a Q-table / MC-returns map key.
///
/// All methods are const — this class holds no mutable state and is
/// safe to share across threads without synchronisation.
class StateEncoder {
public:
    /// Encode with running-count bucket.
    /// Format: "TOTAL|SOFT|UPCARD|COUNTBUCKET"
    /// Example: "16|1|7|+2"
    /// @throws std::logic_error if state fields are out of range.
    std::string encode(const GameStateSnapshot& state) const;

    /// Encode without running count.
    /// Format: "TOTAL|SOFT|UPCARD"
    /// Example: "16|1|7"
    /// @throws std::logic_error if state fields are out of range.
    std::string encodeSimple(const GameStateSnapshot& state) const;

    /// Map a raw running count to one of five bucket labels:
    ///   <= -4  → "-2"
    ///   -3,-2,-1 → "-1"
    ///   0      → "0"
    ///   +1,+2,+3 → "+1"
    ///   >= +4  → "+2"
    /// Bucketing reduces state-space size and forces generalisation
    /// across rarely-visited fine-grained count values.
    std::string countBucket(int runningCount) const;

    /// Total unique states encode() can produce:
    /// 18 totals (4–21) × 2 soft × 10 upcards (2–11) × 5 buckets = 1800.
    static int stateSpaceSize();

private:
    /// @throws std::logic_error on invalid total or upcard.
    void validateState(const GameStateSnapshot& state) const;
};
