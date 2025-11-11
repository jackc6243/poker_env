#pragma once
#include "PokerTypes.hpp"
#include <iomanip>
#include <iostream>
#include <string>

// Helper function to convert card number to string
inline std::string cardNumberToString(uint8_t number) {
  switch (number) {
  case 1:
    return "A";
  case 11:
    return "J";
  case 12:
    return "Q";
  case 13:
    return "K";
  default:
    return std::to_string(number);
  }
}

// Helper function to convert suite to string/symbol
inline std::string suiteToString(uint8_t suite) {
  switch (suite) {
  case 0:
    return "♣"; // Clubs
  case 1:
    return "♦"; // Diamonds
  case 2:
    return "♥"; // Hearts
  case 3:
    return "♠"; // Spades
  default:
    return "?";
  }
}

// Helper function to print a single card
inline std::string cardToString(const Card &card) {
  return cardNumberToString(card.number) + suiteToString(card.suite);
}

// Helper function to convert BettingRound to string
inline std::string bettingRoundToString(BettingRound round) {
  switch (round) {
  case BettingRound::SETUP:
    return "SETUP";
  case BettingRound::PREFLOP:
    return "PREFLOP";
  case BettingRound::FLOP:
    return "FLOP";
  case BettingRound::TURN:
    return "TURN";
  case BettingRound::RIVER:
    return "RIVER";
  case BettingRound::SHOWDOWN:
    return "SHOWDOWN";
  default:
    return "UNKNOWN";
  }
}

// Helper function to convert PlayerState::State to string
inline std::string playerStateToString(PlayerState::State state) {
  switch (state) {
  case PlayerState::ACTIVE:
    return "ACTIVE";
  case PlayerState::INACTIVE:
    return "INACTIVE";
  case PlayerState::FOLDED:
    return "FOLDED";
  case PlayerState::ALL_IN:
    return "ALL_IN";
  default:
    return "UNKNOWN";
  }
}

// Helper function to convert PlayerAction to string
inline std::string playerActionToString(PlayerAction action) {
  switch (action) {
  case PlayerAction::FOLD:
    return "FOLD";
  case PlayerAction::CHECK:
    return "CHECK";
  case PlayerAction::BET:
    return "BET";
  case PlayerAction::CALL:
    return "CALL";
  case PlayerAction::RAISE:
    return "RAISE";
  case PlayerAction::ALL_IN:
    return "ALL_IN";
  default:
    return "UNKNOWN";
  }
}

// Main function to print BoardState
template <int N>
void printBoardState(const BoardState<N> &board, std::ostream &os = std::cout) {
  os << "\n╔════════════════════════════════════════════════════════════════╗"
        "\n";
  os << "║                         BOARD STATE                            ║\n";
  os << "╠════════════════════════════════════════════════════════════════╣\n";

  // Betting round and pot information
  os << "║ Round: " << std::setw(20) << std::left
     << bettingRoundToString(board.betting_round) << " Pot: $" << std::setw(28)
     << std::right << board.pot << " ║\n";
  os << "║ Current Bet: $" << std::setw(15) << std::left << board.current_bet
     << " Blinds: $" << board.small_amount << "/$" << board.big_amount
     << std::setw(20) << " " << " ║\n";

  // Community cards
  os << "╠════════════════════════════════════════════════════════════════╣\n";
  os << "║ Community Cards: ";

  int cardsToShow = 0;
  if (board.betting_round == BettingRound::FLOP)
    cardsToShow = 3;
  else if (board.betting_round == BettingRound::TURN)
    cardsToShow = 4;
  else if (board.betting_round == BettingRound::RIVER ||
           board.betting_round == BettingRound::SHOWDOWN)
    cardsToShow = 5;

  for (int i = 0; i < cardsToShow; i++) {
    os << "[" << cardToString(board.openCards[i]) << "] ";
  }
  for (int i = cardsToShow; i < 5; i++) {
    os << "[  ] ";
  }
  os << std::setw(22 - cardsToShow * 2) << " " << "║\n";

  // Players
  os << "╠════════════════════════════════════════════════════════════════╣\n";
  os << "║                           PLAYERS                              ║\n";
  os << "╠════════════════════════════════════════════════════════════════╣\n";

  for (int i = 0; i < board.n_players; i++) {
    const auto &player = board.players[i];

    // Player header with indicators
    os << "║ Player " << i;
    if (i == board.dealer_button_idx)
      os << " (D)";
    if (i == board.current_player_idx)
      os << " *CURRENT*";
    if (i == board.last_aggressor_idx)
      os << " (AGG)";
    os << std::string(50 - (i >= 10 ? 10 : 9) -
                          (i == board.dealer_button_idx ? 4 : 0) -
                          (i == board.current_player_idx ? 10 : 0) -
                          (i == board.last_aggressor_idx ? 6 : 0),
                      ' ')
       << "║\n";

    // Player cards
    os << "║   Cards: [" << cardToString(player.cards[0]) << "] ["
       << cardToString(player.cards[1]) << "]";
    os << std::setw(42) << " " << "║\n";

    // Player stats
    os << "║   Stack: $" << std::setw(10) << std::left << player.stack
       << " Bet: $" << std::setw(10) << std::left << player.bet
       << " State: " << std::setw(14) << std::left
       << playerStateToString(player.state) << "║\n";

    // Player reward (if any)
    if (board.player_rewards[i] != 0) {
      os << "║   Reward: $" << std::setw(50) << std::left
         << board.player_rewards[i] << "║\n";
    }

    if (i < board.n_players - 1) {
      os << "║" << std::string(64, '-') << "║\n";
    }
  }

  os << "╚════════════════════════════════════════════════════════════════╝\n";
  os << std::endl;
}

// Convenience function for printing to stdout
template <int N> void printBoard(const BoardState<N> &board) {
  printBoardState(board, std::cout);
}
