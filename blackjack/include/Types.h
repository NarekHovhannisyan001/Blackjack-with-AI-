#pragma once

/// Suit of a playing card.
enum class Suit { Hearts, Diamonds, Clubs, Spades };

/// Rank of a playing card, Two through Ace.
enum class Rank {
    Two, Three, Four, Five, Six, Seven,
    Eight, Nine, Ten, Jack, Queen, King, Ace
};

/// Action a player may take on their turn.
enum class PlayerAction { Hit, Stand, Double, Split, Surrender };

/// State machine states for the game engine.
enum class GameState { Betting, Dealing, Insurance, PlayerTurns, DealerTurn, Settling, GameOver };

/// Result of a hand settlement used for payout display.
enum class PayoutResult { Win, Lose, Push, Blackjack, InsuranceWin, InsuranceLose };

constexpr int MAX_PLAYERS          = 4;
constexpr int MIN_BET              = 10;
constexpr int STARTING_CHIPS       = 1000;
constexpr int RESHUFFLE_THRESHOLD  = 52;
constexpr int BLACKJACK_PAYOUT_NUM = 3;
constexpr int BLACKJACK_PAYOUT_DEN = 2;
