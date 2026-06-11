#pragma once

#include "Types.h"

/// One transition in the agent's experience: state → action → reward → next state.
/// Used by Q-Learning for per-step updates and by Monte Carlo to buffer episodes.
struct Experience {
    GameStateSnapshot state;
    PlayerAction      action     = PlayerAction::Stand;
    double            reward     = 0.0;
    GameStateSnapshot nextState;
    bool              isTerminal = false;
};
