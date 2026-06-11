#include "GameEngine.h"
#include "AIAgent.h"
#include "Database.h"
#include "GameRecord.h"
#include <iostream>
#include <stdexcept>
#include <string>

GameEngine::GameEngine(int numPlayers, int numDecks, int startingChips,
                       uint32_t deckSeed, bool aiOnly)
    : m_deck(numDecks)
    , m_state(GameState::Betting)
    , m_database(std::make_unique<Database>())
    , m_roundNumber(0) {
    if (deckSeed != 0) {
        m_deck.shuffle(deckSeed);
    }
    for (int i = 0; i < numPlayers; ++i) {
        std::string name = aiOnly
            ? ("AI_" + std::to_string(i + 1))
            : ("Player " + std::to_string(i + 1));
        m_players.emplace_back(name, startingChips, aiOnly);
    }
    AgentType agentType = aiOnly ? AgentType::QLearning : AgentType::Human;
    m_sessionId = m_database->startSession(agentType, numDecks, startingChips);
}

GameEngine::~GameEngine() = default;

void GameEngine::setAgent(std::unique_ptr<AIAgent> agent) {
    m_agent = std::move(agent);
}

void GameEngine::setHeadless(bool headless) {
    m_headless = headless;
}

GameStateSnapshot GameEngine::buildSnapshot(const Player& player,
                                             int handIndex) const {
    const Hand& hand = player.getHand(handIndex);
    GameStateSnapshot snap;
    snap.playerTotal       = hand.getValue();
    snap.isSoftHand        = hand.isSoft();
    snap.dealerUpcardValue = m_dealer.getHand(0).getCards().empty()
                               ? 2
                               : m_dealer.getHand(0).getCards()[0].getValue();
    snap.runningCount      = m_runningCount;
    snap.handIndex         = handIndex;
    snap.canDouble         = (handIndex == 0)
                           && (hand.cardCount() == 2)
                           && (player.getChips() >= player.getCurrentBet());
    snap.canSplit          = hand.isPair()
                           && (player.getChips() >= player.getCurrentBet())
                           && (player.handCount() == 1);
    snap.canSurrender      = (hand.cardCount() == 2) && !player.hasActed();
    return snap;
}

// ─── Public ──────────────────────────────────────────────────────────────────

void GameEngine::run() {
    if (!m_headless) { m_view.showWelcome(); }
    while (m_state != GameState::GameOver) {
        executeCurrentState();
    }

    // Flush any buffered rounds and close the session record
    m_database->flushBatch();
    int finalChips = m_players.empty() ? 0 : m_players[0].getChips();
    double winRate = m_handsThisSession > 0
        ? static_cast<double>(m_winsThisSession) / m_handsThisSession
        : 0.0;
    m_database->endSession(m_sessionId, finalChips, m_roundNumber, winRate);

    if (!m_headless) {
        for (const auto& p : m_players) {
            m_view.showGameOver(p.getName(), p.getChips());
        }
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
    if (!m_headless) { m_view.showRoundStart(m_roundNumber); }

    if (m_deck.needsReshuffle()) {
        m_deck.reshuffle();
        m_runningCount = 0;
        if (!m_headless) { std::cout << "(Shoe reshuffled)\n"; }
    }

    // Reset per-round action tracking (one entry per player, per hand; splits add entries)
    m_handActions.assign(m_players.size(), std::vector<std::string>(1, "Stand"));

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

// Hi-Lo card counting: 2-6 = +1, 7-9 = 0, 10-A = -1
static void updateHiLo(int& runningCount, const Card& card) {
    int v = card.getValue();
    if (v >= 2 && v <= 6)        { ++runningCount; }
    else if (v >= 10 || v == 1)  { --runningCount; }
    // 7-9 neutral, no change
}

void GameEngine::executeDealing() {
    for (auto& player : m_players) {
        Card c = m_deck.deal();
        updateHiLo(m_runningCount, c);
        player.getHand(0).addCard(c);
    }
    {
        Card c = m_deck.deal();
        updateHiLo(m_runningCount, c);
        m_dealer.getHand(0).addCard(c);
    }
    for (auto& player : m_players) {
        Card c = m_deck.deal();
        updateHiLo(m_runningCount, c);
        player.getHand(0).addCard(c);
    }
    {
        Card c = m_deck.deal();
        updateHiLo(m_runningCount, c);
        m_dealer.getHand(0).addCard(c);
    }
    m_dealer.hideHoleCard();

    if (!m_headless) { m_view.showTable(m_players, m_dealer, true); }
    transitionTo(GameState::Insurance);
}

void GameEngine::executeInsurance() {
    if (m_dealer.getHand(0).getCards().size() >= 1 &&
        m_dealer.getHand(0).getCards()[0].getRank() == Rank::Ace) {
        for (auto& player : m_players) {
            if (!player.isAI() && !m_headless) {
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
    for (int pi = 0; pi < static_cast<int>(m_players.size()); ++pi) {
        int hands = m_players[pi].handCount();
        for (int idx = 0; idx < hands; ++idx) {
            runSingleHandTurn(m_players[pi], pi, idx);
        }
    }
    transitionTo(GameState::DealerTurn);
}

void GameEngine::runSingleHandTurn(Player& player, int playerIndex, int handIndex) {
    if (!m_headless) { m_view.showTable(m_players, m_dealer, true); }

    Hand& hand = player.getHand(handIndex);

    if (hand.isBlackjack()) {
        if (!m_headless) { m_view.showBlackjackMessage(player.getName()); }
        return;
    }

    while (true) {
        PlayerAction action;
        if (player.isAI() && m_agent) {
            GameStateSnapshot snap = buildSnapshot(player, handIndex);
            action = m_agent->decide(snap);
        } else if (player.isAI()) {
            action = PlayerAction::Stand;
        } else {
            action = m_view.promptPlayerAction(player, handIndex);
        }
        player.markActed();

        switch (action) {
            case PlayerAction::Hit: {
                m_handActions[playerIndex][handIndex] = "Hit";
                Card c = m_deck.deal();
                updateHiLo(m_runningCount, c);
                hand.addCard(c);
                if (!m_headless) { m_view.showPlayerHand(player, handIndex); }
                if (hand.isBust()) {
                    if (!m_headless) { m_view.showBustMessage(player.getName()); }
                    return;
                }
                break;
            }
            case PlayerAction::Stand: {
                m_handActions[playerIndex][handIndex] = "Stand";
                return;
            }
            case PlayerAction::Double: {
                m_handActions[playerIndex][handIndex] = "Double";
                player.receiveWinnings(0);
                int extra = player.getCurrentBet();
                player.placeBet(extra);
                Card c = m_deck.deal();
                updateHiLo(m_runningCount, c);
                hand.addCard(c);
                if (!m_headless) { m_view.showPlayerHand(player, handIndex); }
                if (hand.isBust()) {
                    if (!m_headless) { m_view.showBustMessage(player.getName()); }
                }
                return;
            }
            case PlayerAction::Split: {
                m_handActions[playerIndex][handIndex] = "Split";
                bool splitAces =
                    (player.getHand(handIndex).getCards()[0].getRank() == Rank::Ace);
                player.splitHand();
                if (static_cast<int>(m_handActions[playerIndex].size()) < 2) {
                    m_handActions[playerIndex].resize(2, "Stand");
                }
                {
                    Card c0 = m_deck.deal(); updateHiLo(m_runningCount, c0);
                    player.getHand(0).addCard(c0);
                    Card c1 = m_deck.deal(); updateHiLo(m_runningCount, c1);
                    player.getHand(1).addCard(c1);
                }
                if (splitAces) {
                    if (!m_headless) {
                        m_view.showPlayerHand(player, 0);
                        m_view.showPlayerHand(player, 1);
                    }
                } else {
                    runSingleHandTurn(player, playerIndex, 0);
                    runSingleHandTurn(player, playerIndex, 1);
                }
                return;
            }
            case PlayerAction::Surrender: {
                m_handActions[playerIndex][handIndex] = "Surrender";
                player.surrender();
                if (!m_headless) { m_view.showSurrenderMessage(player.getName()); }
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
    if (!m_headless) { m_view.showDealerHand(m_dealer, false); }

    if (!allDone) {
        m_dealer.playTurn(m_deck);
        if (!m_headless) { m_view.showDealerHand(m_dealer, false); }
    }
    transitionTo(GameState::Settling);
}

void GameEngine::executeSettling() {
    int dealerUpcard = m_dealer.getHand(0).getCards().empty()
        ? 0
        : m_dealer.getHand(0).getCards()[0].getValue();

    for (int pi = 0; pi < static_cast<int>(m_players.size()); ++pi) {
        auto& player = m_players[pi];
        int bet = player.getCurrentBet();

        // Insurance settlement
        if (player.getInsuranceBet() > 0) {
            int ins = player.getInsuranceBet();
            if (m_dealer.getHand(0).isBlackjack()) {
                player.receiveWinnings(ins * 3);
                if (!m_headless) {
                    m_view.showPayoutResult(player.getName(),
                        PayoutResult::InsuranceWin, ins, player.getChips());
                }
            } else {
                if (!m_headless) {
                    m_view.showPayoutResult(player.getName(),
                        PayoutResult::InsuranceLose, ins, player.getChips());
                }
            }
        }

        // Surrender half-return
        if (player.hasSurrendered()) {
            int chipsBefore = player.getChips();
            player.receiveWinnings(bet / 2);
            GameRecord rec;
            rec.sessionId         = m_sessionId;
            rec.timestamp         = currentTimestamp();
            rec.playerHand        = serializeHand(player.getHand(0));
            rec.dealerHand        = serializeHand(m_dealer.getHand(0));
            rec.actionTaken       = "Surrender";
            rec.outcome           = "Surrender";
            rec.chipsBefore       = chipsBefore;
            rec.chipsAfter        = player.getChips();
            rec.betAmount         = bet;
            rec.isSoftHand        = player.getHand(0).isSoft();
            rec.dealerUpcardValue = dealerUpcard;
            m_database->insertRound(rec);
            ++m_handsThisSession;
            if (m_database->batchReady()) { m_database->flushBatch(); }
            continue;
        }

        // Settle each hand
        for (int idx = 0; idx < player.handCount(); ++idx) {
            const Hand& ph = player.getHand(idx);
            const Hand& dh = m_dealer.getHand(0);

            std::string actionTaken = (idx < static_cast<int>(m_handActions[pi].size()))
                ? m_handActions[pi][idx]
                : "Stand";

            if (ph.isBust()) {
                if (!m_headless) {
                    m_view.showPayoutResult(player.getName(),
                        PayoutResult::Lose, bet, player.getChips());
                }
                GameRecord rec;
                rec.sessionId         = m_sessionId;
                rec.timestamp         = currentTimestamp();
                rec.playerHand        = serializeHand(ph);
                rec.dealerHand        = serializeHand(dh);
                rec.actionTaken       = actionTaken;
                rec.outcome           = "Lose";
                rec.chipsBefore       = player.getChips();
                rec.chipsAfter        = player.getChips();
                rec.betAmount         = bet;
                rec.isSoftHand        = ph.isSoft();
                rec.dealerUpcardValue = dealerUpcard;
                m_database->insertRound(rec);
                ++m_handsThisSession;
                if (m_database->batchReady()) { m_database->flushBatch(); }
                continue;
            }

            int chipsBefore = player.getChips();
            int payout = calculatePayout(ph, dh, bet);
            player.receiveWinnings(payout);

            PayoutResult res;
            int          display;
            std::string  outcomeStr;
            if (payout == 0) {
                res        = PayoutResult::Lose;
                display    = bet;
                outcomeStr = "Lose";
            } else if (payout == bet) {
                res        = PayoutResult::Push;
                display    = bet;
                outcomeStr = "Push";
            } else if (ph.isBlackjack() && !dh.isBlackjack()) {
                res        = PayoutResult::Blackjack;
                display    = bet * BLACKJACK_PAYOUT_NUM / BLACKJACK_PAYOUT_DEN;
                outcomeStr = "Blackjack";
            } else {
                res        = PayoutResult::Win;
                display    = payout - bet;
                outcomeStr = "Win";
            }
            if (!m_headless) {
                m_view.showPayoutResult(player.getName(), res, display, player.getChips());
            }

            GameRecord rec;
            rec.sessionId         = m_sessionId;
            rec.timestamp         = currentTimestamp();
            rec.playerHand        = serializeHand(ph);
            rec.dealerHand        = serializeHand(dh);
            rec.actionTaken       = actionTaken;
            rec.outcome           = outcomeStr;
            rec.chipsBefore       = chipsBefore;
            rec.chipsAfter        = player.getChips();
            rec.betAmount         = bet;
            rec.isSoftHand        = ph.isSoft();
            rec.dealerUpcardValue = dealerUpcard;
            m_database->insertRound(rec);
            ++m_handsThisSession;
            if (outcomeStr == "Win" || outcomeStr == "Blackjack") { ++m_winsThisSession; }
            if (m_database->batchReady()) { m_database->flushBatch(); }
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

    m_database->flushBatch();

    if (m_headless) {
        // Headless: auto-loop as long as any player can still bet
        if (anyCanPlay) {
            transitionTo(GameState::Betting);
        } else {
            transitionTo(GameState::GameOver);
        }
        return;
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

    // Between-rounds menu: loop until the player makes a terminal choice
    while (true) {
        PostRoundChoice choice = m_view.promptPlayAgain();
        if (choice == PostRoundChoice::ShowHistory) {
            m_database->displayHistory(m_sessionId, MAX_HISTORY_DISPLAY);
        } else if (choice == PostRoundChoice::PlayAgain) {
            transitionTo(GameState::Betting);
            break;
        } else {
            transitionTo(GameState::GameOver);
            break;
        }
    }
}
