#include "GameEngine.h"
#include <iostream>
#include <stdexcept>
#include <string>

GameEngine::GameEngine(int numPlayers, int numDecks, int startingChips,
                       uint32_t deckSeed, bool aiOnly)
    : m_deck(numDecks), m_state(GameState::Betting), m_roundNumber(0) {
    if (deckSeed != 0) {
        m_deck.shuffle(deckSeed);
    }
    for (int i = 0; i < numPlayers; ++i) {
        std::string name = aiOnly
            ? ("AI_" + std::to_string(i + 1))
            : ("Player " + std::to_string(i + 1));
        m_players.emplace_back(name, startingChips, aiOnly);
    }
}

// ─── Public ──────────────────────────────────────────────────────────────────

void GameEngine::run() {
    m_view.showWelcome();
    while (m_state != GameState::GameOver) {
        executeCurrentState();
    }
    for (const auto& p : m_players) {
        m_view.showGameOver(p.getName(), p.getChips());
    }
}

int GameEngine::calculatePayout(const Hand& playerHand,
                                 const Hand& dealerHand, int bet) const {
    bool dealerBust     = dealerHand.isBust();
    bool playerBJ       = playerHand.isBlackjack();
    bool dealerBJ       = dealerHand.isBlackjack();
    bool playerFromSplit = playerHand.isFromSplit();
    int  playerVal      = playerHand.getValue();
    int  dealerVal      = dealerHand.getValue();

    if (dealerBust) {
        return bet * 2;
    }
    if (playerBJ && !dealerBJ) {
        return bet + (bet * BLACKJACK_PAYOUT_NUM / BLACKJACK_PAYOUT_DEN);
    }
    if (playerBJ && dealerBJ) {
        return bet;
    }
    if (playerFromSplit && playerVal == 21) {
        return bet * 2;
    }
    if (playerVal > dealerVal) {
        return bet * 2;
    }
    if (playerVal == dealerVal) {
        return bet;
    }
    return 0;
}

// ─── FSM ─────────────────────────────────────────────────────────────────────

bool GameEngine::isValidTransition(GameState from, GameState to) {
    if (to == GameState::GameOver) { return true; }
    switch (from) {
        case GameState::Betting:      return to == GameState::Dealing;
        case GameState::Dealing:      return to == GameState::Insurance;
        case GameState::Insurance:    return to == GameState::PlayerTurns;
        case GameState::PlayerTurns:  return to == GameState::DealerTurn;
        case GameState::DealerTurn:   return to == GameState::Settling;
        case GameState::Settling:     return to == GameState::Betting;
        case GameState::GameOver:     return false;
    }
    return false;
}

std::string GameEngine::stateToString(GameState s) {
    switch (s) {
        case GameState::Betting:     return "Betting";
        case GameState::Dealing:     return "Dealing";
        case GameState::Insurance:   return "Insurance";
        case GameState::PlayerTurns: return "PlayerTurns";
        case GameState::DealerTurn:  return "DealerTurn";
        case GameState::Settling:    return "Settling";
        case GameState::GameOver:    return "GameOver";
    }
    return "Unknown";
}

void GameEngine::transitionTo(GameState next) {
    if (!isValidTransition(m_state, next)) {
        throw std::logic_error(
            "Illegal FSM transition: " + stateToString(m_state) +
            " -> " + stateToString(next));
    }
#ifdef DEBUG
    std::cerr << "[FSM] " << stateToString(m_state)
              << " -> " << stateToString(next) << "\n";
#endif
    m_state = next;
}

void GameEngine::executeCurrentState() {
    switch (m_state) {
        case GameState::Betting:      executeBetting();      break;
        case GameState::Dealing:      executeDealing();      break;
        case GameState::Insurance:    executeInsurance();    break;
        case GameState::PlayerTurns:  executePlayerTurns();  break;
        case GameState::DealerTurn:   executeDealerTurn();   break;
        case GameState::Settling:     executeSettling();     break;
        case GameState::GameOver:                            break;
    }
}

// ─── Execute methods ─────────────────────────────────────────────────────────

void GameEngine::executeBetting() {
    ++m_roundNumber;
    m_view.showRoundStart(m_roundNumber);

    if (m_deck.needsReshuffle()) {
        m_deck.reshuffle();
        std::cout << "(Shoe reshuffled)\n";
    }

    for (auto& player : m_players) {
        if (player.isAI()) {
            player.placeBet(MIN_BET);
        } else {
            int bet = m_view.promptBet(player);
            player.placeBet(bet);
        }
    }
    transitionTo(GameState::Dealing);
}

void GameEngine::executeDealing() {
    for (auto& player : m_players) {
        player.getHand(0).addCard(m_deck.deal());
    }
    m_dealer.getHand(0).addCard(m_deck.deal());
    for (auto& player : m_players) {
        player.getHand(0).addCard(m_deck.deal());
    }
    m_dealer.getHand(0).addCard(m_deck.deal());
    m_dealer.hideHoleCard();

    m_view.showTable(m_players, m_dealer, true);
    transitionTo(GameState::Insurance);
}

void GameEngine::executeInsurance() {
    if (m_dealer.getHand(0).getCards().size() >= 1 &&
        m_dealer.getHand(0).getCards()[0].getRank() == Rank::Ace) {
        for (auto& player : m_players) {
            if (!player.isAI()) {
                if (m_view.promptInsurance(player)) {
                    int half = player.getCurrentBet() / 2;
                    player.placeInsuranceBet(half > 0 ? half : 1);
                }
            }
        }
    }
    transitionTo(GameState::PlayerTurns);
}

void GameEngine::executePlayerTurns() {
    for (auto& player : m_players) {
        int hands = player.handCount();
        for (int idx = 0; idx < hands; ++idx) {
            runSingleHandTurn(player, idx);
        }
    }
    transitionTo(GameState::DealerTurn);
}

void GameEngine::runSingleHandTurn(Player& player, int handIndex) {
    m_view.showTable(m_players, m_dealer, true);

    Hand& hand = player.getHand(handIndex);

    if (hand.isBlackjack()) {
        m_view.showBlackjackMessage(player.getName());
        return;
    }

    while (true) {
        PlayerAction action;
        if (player.isAI()) {
            action = PlayerAction::Stand;
        } else {
            action = m_view.promptPlayerAction(player, handIndex);
        }
        player.markActed();

        switch (action) {
            case PlayerAction::Hit: {
                hand.addCard(m_deck.deal());
                m_view.showPlayerHand(player, handIndex);
                if (hand.isBust()) {
                    m_view.showBustMessage(player.getName());
                    return;
                }
                break;
            }
            case PlayerAction::Stand: {
                return;
            }
            case PlayerAction::Double: {
                player.receiveWinnings(0);
                int extra = player.getCurrentBet();
                player.placeBet(extra);
                hand.addCard(m_deck.deal());
                m_view.showPlayerHand(player, handIndex);
                if (hand.isBust()) {
                    m_view.showBustMessage(player.getName());
                }
                return;
            }
            case PlayerAction::Split: {
                bool splitAces =
                    (player.getHand(handIndex).getCards()[0].getRank() == Rank::Ace);
                player.splitHand();
                player.getHand(0).addCard(m_deck.deal());
                player.getHand(1).addCard(m_deck.deal());

                if (splitAces) {
                    m_view.showPlayerHand(player, 0);
                    m_view.showPlayerHand(player, 1);
                } else {
                    runSingleHandTurn(player, 0);
                    runSingleHandTurn(player, 1);
                }
                return;
            }
            case PlayerAction::Surrender: {
                player.surrender();
                m_view.showSurrenderMessage(player.getName());
                return;
            }
        }
    }
}

void GameEngine::executeDealerTurn() {
    bool allDone = true;
    for (const auto& player : m_players) {
        if (!player.hasSurrendered()) {
            for (int i = 0; i < player.handCount(); ++i) {
                if (!player.getHand(i).isBust()) {
                    allDone = false;
                }
            }
        }
    }

    m_dealer.revealHoleCard();
    m_view.showDealerHand(m_dealer, false);

    if (!allDone) {
        m_dealer.playTurn(m_deck);
        m_view.showDealerHand(m_dealer, false);
    }
    transitionTo(GameState::Settling);
}

void GameEngine::executeSettling() {
    for (auto& player : m_players) {
        int bet = player.getCurrentBet();

        // Insurance settlement
        if (player.getInsuranceBet() > 0) {
            int ins = player.getInsuranceBet();
            if (m_dealer.getHand(0).isBlackjack()) {
                player.receiveWinnings(ins * 3);
                m_view.showPayoutResult(player.getName(),
                    PayoutResult::InsuranceWin, ins, player.getChips());
            } else {
                m_view.showPayoutResult(player.getName(),
                    PayoutResult::InsuranceLose, ins, player.getChips());
            }
        }

        // Surrender half-return
        if (player.hasSurrendered()) {
            player.receiveWinnings(bet / 2);
            continue;
        }

        // Settle each hand
        for (int idx = 0; idx < player.handCount(); ++idx) {
            const Hand& ph = player.getHand(idx);
            const Hand& dh = m_dealer.getHand(0);

            if (ph.isBust()) {
                m_view.showPayoutResult(player.getName(),
                    PayoutResult::Lose, bet, player.getChips());
                continue;
            }

            int payout = calculatePayout(ph, dh, bet);
            player.receiveWinnings(payout);

            PayoutResult res;
            int          display;
            if (payout == 0) {
                res     = PayoutResult::Lose;
                display = bet;
            } else if (payout == bet) {
                res     = PayoutResult::Push;
                display = bet;
            } else if (ph.isBlackjack() && !dh.isBlackjack()) {
                res     = PayoutResult::Blackjack;
                display = bet * BLACKJACK_PAYOUT_NUM / BLACKJACK_PAYOUT_DEN;
            } else {
                res     = PayoutResult::Win;
                display = payout - bet;
            }
            m_view.showPayoutResult(player.getName(), res, display, player.getChips());
        }
    }

    for (auto& player : m_players) {
        player.resetForNewRound();
    }
    m_dealer.resetForNewRound();

    bool anyCanPlay = false;
    for (const auto& p : m_players) {
        if (p.getChips() >= MIN_BET) {
            anyCanPlay = true;
            break;
        }
    }
    if (!anyCanPlay) {
        transitionTo(GameState::GameOver);
        return;
    }

    bool anyHuman = false;
    for (const auto& p : m_players) {
        if (!p.isAI()) {
            anyHuman = true;
            break;
        }
    }
    if (!anyHuman) {
        transitionTo(GameState::GameOver);
        return;
    }

    if (m_view.promptPlayAgain()) {
        transitionTo(GameState::Betting);
    } else {
        transitionTo(GameState::GameOver);
    }
}
