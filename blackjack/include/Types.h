#pragma once

// ── Playing card enums ────────────────────────────────────────────────────────

enum class Suit { Hearts, Diamonds, Clubs, Spades };

enum class Rank {
    Two, Three, Four, Five, Six, Seven,
    Eight, Nine, Ten, Jack, Queen, King, Ace
};

// ── Game action / state enums ─────────────────────────────────────────────────

enum class PlayerAction { Hit, Stand, Double, Split, Surrender };

enum class GameState { Betting, Dealing, Insurance, PlayerTurns, DealerTurn, Settling, GameOver };

enum class PayoutResult { Win, Lose, Push, Blackjack, InsuranceWin, InsuranceLose };

enum class PostRoundChoice { PlayAgain, ShowHistory, Quit };

// ── Phase 3 enums ─────────────────────────────────────────────────────────────

enum class AgentMode  { Training, Evaluation };
enum class AgentType  { Human, QLearning, MonteCarlo };

// ── Phase 1 / 2 constants ─────────────────────────────────────────────────────

constexpr int MAX_PLAYERS          = 4;
constexpr int MIN_BET              = 10;
constexpr int STARTING_CHIPS       = 1000;
constexpr int RESHUFFLE_THRESHOLD  = 52;
constexpr int BLACKJACK_PAYOUT_NUM = 3;
constexpr int BLACKJACK_PAYOUT_DEN = 2;

constexpr int         BATCH_SIZE          = 1000;
constexpr int         MAX_HISTORY_DISPLAY = 20;
constexpr const char* DB_FILENAME         = "blackjack.db";

// ── Phase 3 constants ─────────────────────────────────────────────────────────

constexpr double ALPHA             = 0.1;
constexpr double GAMMA             = 0.9;
constexpr double EPSILON_START     = 1.0;
constexpr double EPSILON_MIN       = 0.01;
constexpr double EPSILON_EVAL      = 0.0;
constexpr int    TRAINING_EPISODES = 500000;
constexpr int    EVAL_INTERVAL     = 10000;
constexpr int    EVAL_GAMES        = 1000;
constexpr int    DB_SAVE_INTERVAL  = 10000;

constexpr int    COUNT_BUCKET_MIN  = -2;
constexpr int    COUNT_BUCKET_MAX  = +2;

// ── Phase 3 reward constants ──────────────────────────────────────────────────

constexpr double REWARD_WIN        = +1.0;
constexpr double REWARD_BLACKJACK  = +1.5;
constexpr double REWARD_LOSE       = -1.0;
constexpr double REWARD_PUSH       =  0.0;
constexpr double REWARD_SURRENDER  = -0.5;

// ── GameStateSnapshot ─────────────────────────────────────────────────────────

/// Read-only snapshot of the game passed to an AI agent before it decides.
/// Plain struct — no methods, all public.
struct GameStateSnapshot {
    int  playerTotal       = 0;
    bool isSoftHand        = false;
    int  dealerUpcardValue = 0;   // 2–11 (Ace = 11)
    int  runningCount      = 0;   // Hi-Lo running count, unbounded
    bool canDouble         = false;
    bool canSplit          = false;
    bool canSurrender      = false;
    int  handIndex         = 0;   // 0 = main, 1 = split hand
};
