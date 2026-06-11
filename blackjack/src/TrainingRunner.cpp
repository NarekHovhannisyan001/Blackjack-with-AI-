#include "TrainingRunner.h"
#include "AIAgent.h"
#include "Card.h"
#include "Database.h"
#include "Deck.h"
#include "Hand.h"
#include "PolicyManager.h"
#include "TrainingRecord.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ── Local helpers ─────────────────────────────────────────────────────────────

static void hiLo(const Card& card, int& count) {
    int v = card.getValue();
    if (v >= 2 && v <= 6)       { ++count; }
    else if (v >= 10 || v == 1) { --count; }
}

static GameStateSnapshot makeSnap(const Hand& hand, int dealerUpcard,
                                   int runningCount,
                                   bool canDouble, bool canSplit, bool canSurrender) {
    GameStateSnapshot s;
    s.playerTotal       = hand.getValue();
    s.isSoftHand        = hand.isSoft();
    s.dealerUpcardValue = dealerUpcard;
    s.runningCount      = runningCount;
    s.canDouble         = canDouble;
    s.canSplit          = canSplit;
    s.canSurrender      = canSurrender;
    s.handIndex         = 0;
    return s;
}

static std::string nowTimestamp() {
    auto t = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ── Construction / Destruction ────────────────────────────────────────────────

TrainingRunner::TrainingRunner(AgentType type, int numDecks, const std::string& dbPath)
    : m_agent(PolicyManager::createAgent(type))
    , m_db(std::make_unique<Database>(dbPath))
    , m_numDecks(numDecks) {
    m_episodeBuf.reserve(16);
    m_sessionId = m_db->startSession(type, numDecks, 0);
    m_agent->loadPolicy(*m_db);
}

TrainingRunner::~TrainingRunner() = default;

// ── Accessors ─────────────────────────────────────────────────────────────────

int    TrainingRunner::episodesRun() const { return m_episodesRun; }
int    TrainingRunner::updateCount() const { return m_agent->updateCount(); }
double TrainingRunner::lastEpsilon() const { return m_agent->currentEpsilon(); }

// ── Inner simulation ──────────────────────────────────────────────────────────

double TrainingRunner::simulateEpisode(Deck& deck) {
    m_episodeBuf.clear();

    // Deal two cards each
    Hand playerHand, dealerHand;
    { Card c = deck.deal(); hiLo(c, m_runningCount); playerHand.addCard(c); }
    { Card c = deck.deal(); hiLo(c, m_runningCount); dealerHand.addCard(c); }
    { Card c = deck.deal(); hiLo(c, m_runningCount); playerHand.addCard(c); }
    { Card c = deck.deal(); hiLo(c, m_runningCount); dealerHand.addCard(c); }

    int dealerUpcard = dealerHand.getCards()[0].getValue();

    // Instant blackjack resolution
    bool pBJ = playerHand.isBlackjack();
    bool dBJ = dealerHand.isBlackjack();
    if (pBJ || dBJ) {
        double r = (pBJ && !dBJ) ? REWARD_BLACKJACK
                 : (!pBJ && dBJ) ? REWARD_LOSE
                 : REWARD_PUSH;
        GameStateSnapshot snap = makeSnap(playerHand, dealerUpcard, m_runningCount,
                                          false, false, false);
        m_episodeBuf.push_back({snap, PlayerAction::Stand, r, snap, true});
        for (auto& e : m_episodeBuf) { m_agent->learn(e); }
        m_agent->learnEpisode(m_episodeBuf);
        return r;
    }

    // Player turn
    bool needsDealer = true;
    while (true) {
        bool isFirst   = (playerHand.cardCount() == 2);
        bool canDouble = isFirst;
        bool canSplit  = isFirst && playerHand.isPair();
        bool canSurr   = isFirst;

        GameStateSnapshot snap = makeSnap(playerHand, dealerUpcard, m_runningCount,
                                          canDouble, canSplit, canSurr);
        PlayerAction action = m_agent->decide(snap);

        if (action == PlayerAction::Surrender) {
            m_episodeBuf.push_back({snap, action, REWARD_SURRENDER, snap, true});
            needsDealer = false;
            break;
        }

        if (action == PlayerAction::Stand) {
            GameStateSnapshot next = makeSnap(playerHand, dealerUpcard, m_runningCount,
                                               false, false, false);
            m_episodeBuf.push_back({snap, action, 0.0, next, true}); // reward set after dealer
            break;
        }

        if (action == PlayerAction::Double) {
            Card c = deck.deal(); hiLo(c, m_runningCount); playerHand.addCard(c);
            GameStateSnapshot next = makeSnap(playerHand, dealerUpcard, m_runningCount,
                                               false, false, false);
            if (playerHand.isBust()) {
                m_episodeBuf.push_back({snap, action, REWARD_LOSE, next, true});
                needsDealer = false;
            } else {
                m_episodeBuf.push_back({snap, action, 0.0, next, true}); // reward set after dealer
            }
            break;
        }

        // Hit (or Split treated as Hit)
        PlayerAction stored = (action == PlayerAction::Split) ? PlayerAction::Hit : action;
        Card c = deck.deal(); hiLo(c, m_runningCount); playerHand.addCard(c);
        GameStateSnapshot next = makeSnap(playerHand, dealerUpcard, m_runningCount,
                                           false, false, false);
        if (playerHand.isBust()) {
            m_episodeBuf.push_back({snap, stored, REWARD_LOSE, next, true});
            needsDealer = false;
            break;
        }
        m_episodeBuf.push_back({snap, stored, 0.0, next, false});
        // continue loop
    }

    // Dealer turn
    if (needsDealer) {
        while (dealerHand.getValue() < 17 ||
               (dealerHand.isSoft() && dealerHand.getValue() == 17)) {
            Card c = deck.deal(); hiLo(c, m_runningCount); dealerHand.addCard(c);
        }
        double r;
        if (dealerHand.isBust() || playerHand.getValue() > dealerHand.getValue()) {
            r = REWARD_WIN;
        } else if (playerHand.getValue() == dealerHand.getValue()) {
            r = REWARD_PUSH;
        } else {
            r = REWARD_LOSE;
        }
        if (!m_episodeBuf.empty()) { m_episodeBuf.back().reward = r; }
    }

    double finalReward = m_episodeBuf.empty() ? 0.0 : m_episodeBuf.back().reward;
    for (auto& e : m_episodeBuf) { m_agent->learn(e); }
    m_agent->learnEpisode(m_episodeBuf);
    return finalReward;
}

// ── Main training loop ────────────────────────────────────────────────────────

void TrainingRunner::run(int episodes) {
    Deck deck(m_numDecks);
    deck.shuffle(0);

    int    wins      = 0;
    double totalReward = 0.0;
    std::string agentLabel =
        (m_agent->getMode() == AgentMode::Training) ? "agent" : "agent";

    auto startTime = std::chrono::steady_clock::now();

    for (int ep = 0; ep < episodes; ++ep) {
        if (deck.needsReshuffle()) {
            deck.reshuffle();
            m_runningCount = 0;
        }

        double reward = simulateEpisode(deck);
        totalReward += reward;
        if (reward > 0.0) { ++wins; }
        ++m_episodesRun;

        // Periodic progress + DB snapshot
        if ((ep + 1) % EVAL_INTERVAL == 0) {
            auto now     = std::chrono::steady_clock::now();
            double secs  = std::chrono::duration<double>(now - startTime).count();
            double rate  = (ep + 1) / secs;
            double winPct = static_cast<double>(wins) / (ep + 1) * 100.0;
            double avgR   = totalReward / (ep + 1);

            std::cout << "[" << (ep + 1) << "/" << episodes << "] "
                      << "eps/s=" << static_cast<int>(rate)
                      << " eps=" << std::fixed << std::setprecision(4)
                      << m_agent->currentEpsilon()
                      << " win=" << std::setprecision(1) << winPct
                      << "% avgR=" << std::setprecision(3) << avgR
                      << "\n";

            TrainingRecord tr;
            tr.sessionId    = m_sessionId;
            tr.timestamp    = nowTimestamp();
            tr.agentType    = agentLabel;
            tr.totalEpisodes = ep + 1;
            tr.winRate      = winPct / 100.0;
            tr.avgReward    = avgR;
            tr.epsilon      = m_agent->currentEpsilon();
            tr.qTableSize   = 0;
            m_db->insertTrainingSnapshot(tr);
        }

        if ((ep + 1) % DB_SAVE_INTERVAL == 0) {
            m_agent->savePolicy(*m_db);
        }
    }

    // Final save
    m_agent->savePolicy(*m_db);

    auto endTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - startTime).count();
    double rate    = (elapsed > 0.0) ? (episodes / elapsed) : 0.0;

    std::cout << "Done: " << episodes << " episodes in "
              << std::fixed << std::setprecision(2) << elapsed << "s ("
              << static_cast<int>(rate) << " eps/s)\n";
}
