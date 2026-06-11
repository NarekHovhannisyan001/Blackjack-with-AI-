#include "ConsoleView.h"
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

static const std::string SEPARATOR =
    "--------------------------------------------------";

std::string ConsoleView::formatHand(const Hand& hand, bool hideSecondCard) const {
    const auto& cards = hand.getCards();
    std::string result;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) {
            result += " ";
        }
        if (hideSecondCard && i == cards.size() - 1) {
            result += "[?]";
        } else {
            result += "[" + cards[i].toShortString() + "]";
        }
    }
    return result;
}

void ConsoleView::showWelcome() {
    std::cout << "==================================================\n";
    std::cout << "         BLACKJACK WITH AI — PHASE 1\n";
    std::cout << "==================================================\n";
}

void ConsoleView::showRoundStart(int roundNumber) {
    std::cout << "\n====== ROUND " << roundNumber << " ======\n";
}

void ConsoleView::showRoundSeparator() {
    std::cout << SEPARATOR << "\n";
}

void ConsoleView::showTable(const std::vector<Player>& players,
                             const Dealer& dealer, bool hideHole) {
    showRoundSeparator();
    showDealerHand(dealer, hideHole);
    for (const auto& player : players) {
        for (int i = 0; i < player.handCount(); ++i) {
            showPlayerHand(player, i);
        }
    }
    showRoundSeparator();
}

void ConsoleView::showDealerHand(const Dealer& dealer, bool hideHole) {
    std::string cards = formatHand(dealer.getHand(0), hideHole);
    std::string line = "Dealer:  " + cards;
    if (!hideHole && !dealer.getHand(0).getCards().empty()) {
        line += "   = " + std::to_string(dealer.getHand(0).getValue());
    }
    std::cout << line << "\n";
}

void ConsoleView::showPlayerHand(const Player& player, int handIndex) {
    const Hand& hand = player.getHand(handIndex);
    std::string label = player.getName() + ":";
    while (static_cast<int>(label.size()) < 9) {
        label += " ";
    }
    std::string line = label + " " + formatHand(hand);
    if (!hand.getCards().empty() && !hand.isBust()) {
        line += "   = " + std::to_string(hand.getValue());
    }
    if (hand.isBlackjack()) {
        line += "  *** BLACKJACK ***";
    } else if (hand.isBust()) {
        line += "   BUST";
    }
    std::cout << line << "\n";
}

void ConsoleView::showBustMessage(const std::string& name) {
    std::cout << name << " busts!\n";
}

void ConsoleView::showBlackjackMessage(const std::string& name) {
    std::cout << name << " — BLACKJACK!\n";
}

void ConsoleView::showSurrenderMessage(const std::string& name) {
    std::cout << name << " surrenders (half bet returned).\n";
}

void ConsoleView::showPayoutResult(const std::string& name,
                                    PayoutResult result, int amount, int newTotal) {
    switch (result) {
        case PayoutResult::Win:
            std::cout << name << " wins $" << amount
                      << "  (chips: " << newTotal << ")\n";
            break;
        case PayoutResult::Blackjack:
            std::cout << name << " wins $" << amount
                      << " \xe2\x80\x94 Blackjack! (chips: " << newTotal << ")\n";
            break;
        case PayoutResult::Push:
            std::cout << name << " pushes \xe2\x80\x94 bet returned (chips: "
                      << newTotal << ")\n";
            break;
        case PayoutResult::Lose:
            std::cout << name << " loses $" << amount
                      << "  (chips: " << newTotal << ")\n";
            break;
        case PayoutResult::InsuranceWin:
            std::cout << name << " wins insurance $" << amount
                      << " (chips: " << newTotal << ")\n";
            break;
        case PayoutResult::InsuranceLose:
            std::cout << name << " loses insurance (chips: " << newTotal << ")\n";
            break;
    }
}

void ConsoleView::showInsuranceOffer() {
    std::cout << "Dealer shows an Ace. Insurance? (y/n): ";
}

void ConsoleView::showGameOver(const std::string& name, int finalChips) {
    std::cout << "\n" << SEPARATOR << "\n";
    std::cout << "Game over — " << name << " finishes with $" << finalChips << ".\n";
}

PlayerAction ConsoleView::promptPlayerAction(const Player& player, int handIndex) {
    const Hand& hand = player.getHand(handIndex);
    bool canDouble   = (hand.cardCount() == 2 && player.getChips() >= player.getCurrentBet());
    bool canSplit    = (hand.isPair()          && player.getChips() >= player.getCurrentBet());
    bool canSurrender = (hand.cardCount() == 2 && !player.hasActed());

    while (true) {
        std::string prompt = "Your move \xe2\x80\x94 [H]it  [S]tand";
        if (canDouble)    { prompt += "  [D]ouble"; }
        if (canSplit)     { prompt += "  [P]split"; }
        if (canSurrender) { prompt += "  [R]surrender"; }
        prompt += ": ";
        std::cout << prompt;

        char c = '\0';
        std::cin >> c;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input, please try again.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (c == 'h') { return PlayerAction::Hit; }
        if (c == 's') { return PlayerAction::Stand; }
        if (c == 'd' && canDouble)    { return PlayerAction::Double; }
        if (c == 'p' && canSplit)     { return PlayerAction::Split; }
        if (c == 'r' && canSurrender) { return PlayerAction::Surrender; }

        std::cout << "Invalid input, please try again.\n";
    }
}

int ConsoleView::promptBet(const Player& player) {
    while (true) {
        std::cout << "Your bet (chips: " << player.getChips()
                  << ", min " << MIN_BET << "): ";
        int amount = 0;
        std::cin >> amount;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input, please try again.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (amount >= MIN_BET && amount <= player.getChips()) {
            return amount;
        }
        std::cout << "Bet must be between " << MIN_BET
                  << " and " << player.getChips() << ". Please try again.\n";
    }
}

bool ConsoleView::promptInsurance(const Player& player) {
    showInsuranceOffer();
    (void)player;
    while (true) {
        char c = '\0';
        std::cin >> c;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter y or n: ";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (c == 'y') { return true; }
        if (c == 'n') { return false; }
        std::cout << "Please enter y or n: ";
    }
}

PostRoundChoice ConsoleView::promptPlayAgain() {
    std::cout << "\n[Y]es play again  [H]istory  [N]o quit: ";
    while (true) {
        char c = '\0';
        std::cin >> c;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter y, h, or n: ";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (c == 'y') { return PostRoundChoice::PlayAgain; }
        if (c == 'n') { return PostRoundChoice::Quit; }
        if (c == 'h') { return PostRoundChoice::ShowHistory; }
        std::cout << "Please enter y, h, or n: ";
    }
}
