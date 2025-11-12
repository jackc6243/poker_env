#include "../src/Board.cpp"
#include "../src/PokerTypes.hpp"
#include <gtest/gtest.h>

class PokerBoardComprehensiveTest : public ::testing::Test {
protected:
  PokerBoard<6> board;

  void SetUp() override { board.hard_reset(); }

  // Helper to setup a basic 2-player game ready for action
  void setupTwoPlayerGame() {
    board.add_player(1000);
    board.add_player(1000);
    board.auto_start();
  }

  // Helper to setup a 3-player game
  void setupThreePlayerGame() {
    board.add_player(1000);
    board.add_player(1000);
    board.add_player(1000);
    board.auto_start();
  }

  // Helper to advance to a specific betting round
  void advanceToFlop() {
    auto state = board.get_board_state();
    while (state.betting_round == BettingRound::PREFLOP) {
      state = board.get_board_state();
      int acting = state.current_player_idx;

      if (state.players[acting].bet < state.current_bet) {
        Action call{static_cast<uint8_t>(acting), PlayerAction::CALL, 0};
        board.player_act(call);
      } else {
        Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
        board.player_act(check);
      }
    }
  }

  void advanceToTurn() {
    advanceToFlop();
    auto state = board.get_board_state();
    while (state.betting_round == BettingRound::FLOP) {
      state = board.get_board_state();
      int acting = state.current_player_idx;
      Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
      board.player_act(check);
    }
  }

  void advanceToRiver() {
    advanceToTurn();
    auto state = board.get_board_state();
    while (state.betting_round == BettingRound::TURN) {
      state = board.get_board_state();
      int acting = state.current_player_idx;
      Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
      board.player_act(check);
    }
  }
};

// ===== Player Management Tests =====

TEST_F(PokerBoardComprehensiveTest, AddMultiplePlayersSequentially) {
  for (int i = 0; i < 6; i++) {
    int idx = board.add_player(1000 + i * 100);
    EXPECT_EQ(idx, i);

    auto state = board.get_board_state();
    EXPECT_EQ(state.n_players, i + 1);
    EXPECT_EQ(state.players[i].stack, 1000 + i * 100);
  }
}

TEST_F(PokerBoardComprehensiveTest, AddPlayerWithZeroStack) {
  int idx = board.add_player(0);
  EXPECT_EQ(idx, 0);

  auto state = board.get_board_state();
  EXPECT_EQ(state.players[0].stack, 0);
}

TEST_F(PokerBoardComprehensiveTest, AddPlayerDuringHandFails) {
  setupTwoPlayerGame();
  EXPECT_THROW(board.add_player(1000), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, RemovePlayerDuringHandFails) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  EXPECT_THROW(board.remove_player(0), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, RemoveMiddlePlayer) {
  board.add_player(1000);
  board.add_player(2000);
  board.add_player(3000);
  board.add_player(4000);

  PlayerState removed = board.remove_player(1);
  EXPECT_EQ(removed.stack, 2000);

  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 3);
  EXPECT_EQ(state.players[0].stack, 1000);
  EXPECT_EQ(state.players[1].stack, 3000);
  EXPECT_EQ(state.players[2].stack, 4000);
}

TEST_F(PokerBoardComprehensiveTest, RemoveFirstPlayer) {
  board.add_player(1000);
  board.add_player(2000);
  board.add_player(3000);

  PlayerState removed = board.remove_player(0);
  EXPECT_EQ(removed.stack, 1000);

  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 2);
  EXPECT_EQ(state.players[0].stack, 2000);
  EXPECT_EQ(state.players[1].stack, 3000);
}

TEST_F(PokerBoardComprehensiveTest, RemoveLastPlayer) {
  board.add_player(1000);
  board.add_player(2000);
  board.add_player(3000);

  PlayerState removed = board.remove_player(2);
  EXPECT_EQ(removed.stack, 3000);

  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 2);
  EXPECT_EQ(state.players[0].stack, 1000);
  EXPECT_EQ(state.players[1].stack, 2000);
}

TEST_F(PokerBoardComprehensiveTest, RemoveInvalidPlayerIndex) {
  board.add_player(1000);

  EXPECT_THROW(board.remove_player(5), std::runtime_error);
  EXPECT_THROW(board.remove_player(-1), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, StackTopupMultipleTimes) {
  int idx = board.add_player(1000);

  int stack1 = board.player_stack_topup(idx, 500);
  EXPECT_EQ(stack1, 1500);

  int stack2 = board.player_stack_topup(idx, 300);
  EXPECT_EQ(stack2, 1800);

  int stack3 = board.player_stack_topup(idx, 200);
  EXPECT_EQ(stack3, 2000);
}

TEST_F(PokerBoardComprehensiveTest, ActivateDeactivatePlayer) {
  int idx = board.add_player(1000);

  auto state1 = board.get_board_state();
  EXPECT_EQ(state1.players[idx].state, PlayerState::State::ACTIVE);

  board.deactivate_player(idx);
  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.players[idx].state, PlayerState::State::INACTIVE);

  board.activate_player(idx);
  auto state3 = board.get_board_state();
  EXPECT_EQ(state3.players[idx].state, PlayerState::State::ACTIVE);
}

TEST_F(PokerBoardComprehensiveTest, DeactivateAlreadyInactivePlayer) {
  int idx = board.add_player(1000);
  board.deactivate_player(idx);

  EXPECT_NO_THROW(board.deactivate_player(idx));
  auto state = board.get_board_state();
  EXPECT_EQ(state.players[idx].state, PlayerState::State::INACTIVE);
}

TEST_F(PokerBoardComprehensiveTest, ActivateAlreadyActivePlayer) {
  int idx = board.add_player(1000);

  EXPECT_NO_THROW(board.activate_player(idx));
  auto state = board.get_board_state();
  EXPECT_EQ(state.players[idx].state, PlayerState::State::ACTIVE);
}

// ===== Hand Setup Tests =====

TEST_F(PokerBoardComprehensiveTest, StartHandFromSetup) {
  board.add_player(1000);
  board.add_player(1000);

  auto state1 = board.get_board_state();
  EXPECT_EQ(state1.betting_round, BettingRound::SETUP);

  board.auto_start();
  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.betting_round, BettingRound::PREFLOP);
}

TEST_F(PokerBoardComprehensiveTest, StartHandNotFromSetupFails) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  // Can't call auto_start again while hand is in progress
  EXPECT_THROW(board.auto_start(), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, ResetHandWithNoPlayers) {
  EXPECT_THROW(board.auto_start(), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, DealerButtonAdvances) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);

  auto state1 = board.get_board_state();
  int dealer1 = state1.dealer_button_idx;
  EXPECT_EQ(dealer1, -1); // Initial position before any hand

  board.auto_start();
  auto state2 = board.get_board_state();
  int dealer2 = state2.dealer_button_idx;
  EXPECT_EQ(dealer2, 0); // First hand starts at position 0
}

TEST_F(PokerBoardComprehensiveTest, ManualHandReset) {
  board.add_player(1000);
  board.add_player(1000);

  std::array<Card, 5> open_cards;
  for (int i = 0; i < 5; i++) {
    open_cards[i] = Card{static_cast<uint8_t>(i + 1)};
  }

  std::vector<std::array<Card, 2>> player_cards(2);
  player_cards[0][0] = Card{10};
  player_cards[0][1] = Card{11};
  player_cards[1][0] = Card{20};
  player_cards[1][1] = Card{21};

  board.auto_start(open_cards, player_cards);

  auto state = board.get_board_state();
  EXPECT_EQ(state.openCards[0].number, 1);
  EXPECT_EQ(state.players[0].cards[0].number, 10);
  EXPECT_EQ(state.players[1].cards[1].number, 21);
  EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);
}

TEST_F(PokerBoardComprehensiveTest, ManualHandResetMismatchedPlayerCount) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);

  std::array<Card, 5> open_cards;
  std::vector<std::array<Card, 2>> player_cards(2); // Only 2 player cards

  EXPECT_THROW(board.auto_start(open_cards, player_cards), std::runtime_error);
}

// ===== Blind Posting Tests =====

TEST_F(PokerBoardComprehensiveTest, PostBlindsHeadsUpPositions) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state2 = board.get_board_state();
  int dealer = state2.dealer_button_idx;

  // In heads-up, dealer is small blind
  EXPECT_EQ(state2.players[dealer].bet, 10);
  EXPECT_EQ(state2.players[(dealer + 1) % 2].bet, 20);
  EXPECT_EQ(state2.current_player_idx, dealer); // Dealer acts first in heads-up
}

TEST_F(PokerBoardComprehensiveTest, PostBlindsMultiPlayerPositions) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state2 = board.get_board_state();
  int dealer = state2.dealer_button_idx;

  int small_blind_idx = (dealer + 1) % 4;
  int big_blind_idx = (dealer + 2) % 4;
  int utg_idx = (dealer + 3) % 4;

  EXPECT_EQ(state2.players[small_blind_idx].bet, 10);
  EXPECT_EQ(state2.players[big_blind_idx].bet, 20);
  EXPECT_EQ(state2.current_player_idx, utg_idx);
  EXPECT_EQ(state2.last_aggressor_idx, big_blind_idx);
}

TEST_F(PokerBoardComprehensiveTest, PostBlindsWithOnePlayer) {
  board.add_player(1000);

  EXPECT_THROW(board.auto_start(), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, PostBlindsSmallBlindInsufficientChips) {
  board.add_player(5); // Not enough for small blind
  board.add_player(1000);

  EXPECT_THROW(board.auto_start(), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, PostBlindsBigBlindInsufficientChips) {
  board.add_player(1000);
  board.add_player(15); // Enough for SB but not BB

  EXPECT_THROW(board.auto_start(), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, PostBlindsExactChips) {
  board.add_player(10); // Exactly small blind (dealer in heads-up)
  board.add_player(20); // Exactly big blind

  EXPECT_NO_THROW(board.auto_start());

  auto state = board.get_board_state();
  int dealer = state.dealer_button_idx;
  EXPECT_EQ(state.players[dealer].stack,
            0); // Dealer is small blind in heads-up, all-in
  EXPECT_EQ(state.players[(dealer + 1) % 2].stack, 0); // Big blind all-in
}

// ===== Action Validation Tests =====

TEST_F(PokerBoardComprehensiveTest, CheckWhenFacingBetFails) {
  setupTwoPlayerGame();

  auto state = board.get_board_state();
  int acting = state.current_player_idx;

  // Player facing a bet cannot check
  Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
  EXPECT_THROW(board.player_act(check), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, BetWhenFacingBetFails) {
  setupTwoPlayerGame();

  auto state = board.get_board_state();
  int acting = state.current_player_idx;

  // Player facing a bet cannot bet, must call or raise
  Action bet{static_cast<uint8_t>(acting), PlayerAction::BET, 50};
  EXPECT_THROW(board.player_act(bet), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, RaiseTooSmallFails) {
  setupTwoPlayerGame();

  auto state = board.get_board_state();
  int acting = state.current_player_idx;

  // Raise must be higher than current bet
  Action raise{static_cast<uint8_t>(acting), PlayerAction::RAISE,
               20}; // Same as current bet
  EXPECT_THROW(board.player_act(raise), std::runtime_error);
}

TEST_F(PokerBoardComprehensiveTest, ActionWithInsufficientChips) {
  board.add_player(25); // Only 25 chips
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  int acting = state.current_player_idx;

  // If the small-stack player is acting and doesn't have enough to raise to 100
  if (state.players[acting].stack + state.players[acting].bet < 100) {
    Action raise{static_cast<uint8_t>(acting), PlayerAction::RAISE, 100};
    EXPECT_THROW(board.player_act(raise), std::runtime_error);
  }
}

TEST_F(PokerBoardComprehensiveTest, InactivePlayerRemovedOnAutoStart) {
  board.add_player(1000);
  board.add_player(1000);
  int idx = board.add_player(1000);

  auto state1 = board.get_board_state();
  EXPECT_EQ(state1.n_players, 3);

  board.deactivate_player(idx);

  // auto_start removes inactive players before starting
  board.auto_start();

  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.n_players, 2); // Inactive player was removed
  EXPECT_EQ(state2.betting_round, BettingRound::PREFLOP);
}

// ===== Betting Round Progression Tests =====

TEST_F(PokerBoardComprehensiveTest, PreflopToFlop) {
  setupTwoPlayerGame();
  advanceToFlop();

  auto state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::FLOP);
}

TEST_F(PokerBoardComprehensiveTest, FlopToTurn) {
  setupThreePlayerGame();
  advanceToTurn();

  auto state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::TURN);
}

TEST_F(PokerBoardComprehensiveTest, TurnToRiver) {
  setupThreePlayerGame();
  advanceToRiver();

  auto state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::RIVER);
}

TEST_F(PokerBoardComprehensiveTest, RiverToShowdown) {
  setupThreePlayerGame();
  advanceToRiver();

  auto state = board.get_board_state();
  while (state.betting_round == BettingRound::RIVER) {
    state = board.get_board_state();
    int acting = state.current_player_idx;
    Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
    auto result = board.player_act(check);
    if (!result.has_value())
      break;
  }

  state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::SETUP); // Hand completed
}

TEST_F(PokerBoardComprehensiveTest, BettingResetsBetweenRounds) {
  setupThreePlayerGame();

  auto state1 = board.get_board_state();
  EXPECT_GT(state1.current_bet, 0); // Blinds posted

  advanceToFlop();
  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.current_bet, 20); // Bets carry over but are still in place

  // All players check on flop
  while (state2.betting_round == BettingRound::FLOP) {
    state2 = board.get_board_state();
    int acting = state2.current_player_idx;
    Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
    board.player_act(check);
  }

  // On turn, betting resets but accumulated bets remain
  auto state3 = board.get_board_state();
  EXPECT_EQ(state3.betting_round, BettingRound::TURN);
}

// ===== Multiple Action Sequences Tests =====

TEST_F(PokerBoardComprehensiveTest, CallRaiseReraiseSequence) {
  setupThreePlayerGame();

  auto state = board.get_board_state();

  // Player 1 calls
  int player1 = state.current_player_idx;
  Action call{static_cast<uint8_t>(player1), PlayerAction::CALL, 0};
  board.player_act(call);

  // Player 2 raises
  state = board.get_board_state();
  int player2 = state.current_player_idx;
  Action raise{static_cast<uint8_t>(player2), PlayerAction::RAISE, 60};
  board.player_act(raise);

  state = board.get_board_state();
  EXPECT_EQ(state.current_bet, 60);
  EXPECT_EQ(state.last_aggressor_idx, player2);

  // Player 3 (big blind) reraises
  state = board.get_board_state();
  int player3 = state.current_player_idx;
  Action reraise{static_cast<uint8_t>(player3), PlayerAction::RAISE, 120};
  board.player_act(reraise);

  state = board.get_board_state();
  EXPECT_EQ(state.current_bet, 120);
  EXPECT_EQ(state.last_aggressor_idx, player3);
}

TEST_F(PokerBoardComprehensiveTest, BetCallCallOnFlop) {
  setupThreePlayerGame();
  advanceToFlop();

  auto state = board.get_board_state();

  // Player 1 bets
  int player1 = state.current_player_idx;
  int player1_bet_before = state.players[player1].bet;
  Action bet{static_cast<uint8_t>(player1), PlayerAction::BET, 50};
  board.player_act(bet);

  state = board.get_board_state();
  EXPECT_EQ(state.current_bet, player1_bet_before + 50);

  // Player 2 calls
  int player2 = state.current_player_idx;
  Action call1{static_cast<uint8_t>(player2), PlayerAction::CALL, 0};
  board.player_act(call1);

  // Player 3 calls
  state = board.get_board_state();
  int player3 = state.current_player_idx;
  Action call2{static_cast<uint8_t>(player3), PlayerAction::CALL, 0};
  board.player_act(call2);

  // Should advance to turn
  state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::TURN);
}

TEST_F(PokerBoardComprehensiveTest, CheckCheckCheckAdvancesRound) {
  setupThreePlayerGame();
  advanceToFlop();

  auto state1 = board.get_board_state();
  EXPECT_EQ(state1.betting_round, BettingRound::FLOP);

  // All check
  for (int i = 0; i < 3; i++) {
    auto state = board.get_board_state();
    if (state.betting_round != BettingRound::FLOP)
      break;

    int acting = state.current_player_idx;
    Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
    board.player_act(check);
  }

  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.betting_round, BettingRound::TURN);
}

// ===== All-In Scenarios =====

TEST_F(PokerBoardComprehensiveTest, AllInPreflop) {
  board.add_player(100);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  int short_stack = -1;
  for (int i = 0; i < 2; i++) {
    if (state.players[i].stack + state.players[i].bet <= 100) {
      short_stack = i;
      break;
    }
  }

  if (short_stack != -1 && state.current_player_idx == short_stack) {
    Action all_in{static_cast<uint8_t>(short_stack), PlayerAction::ALL_IN, 0};
    auto result = board.player_act(all_in);

    if (result.has_value()) {
      state = board.get_board_state();
      EXPECT_EQ(state.players[short_stack].state, PlayerState::State::ALL_IN);
      EXPECT_EQ(state.players[short_stack].stack, 0);
    }
  }
}

TEST_F(PokerBoardComprehensiveTest, MultipleAllIns) {
  board.add_player(100);
  board.add_player(200);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();

  // Force players all-in by having them raise/call with their stacks
  while (state.betting_round == BettingRound::PREFLOP) {
    state = board.get_board_state();
    int acting = state.current_player_idx;

    if (state.players[acting].stack <= 100) {
      Action all_in{static_cast<uint8_t>(acting), PlayerAction::ALL_IN, 0};
      auto result = board.player_act(all_in);
      if (!result.has_value())
        break;
    } else {
      Action call{static_cast<uint8_t>(acting), PlayerAction::CALL, 0};
      auto result = board.player_act(call);
      if (!result.has_value())
        break;
    }
  }

  // Hand should complete or advance
  state = board.get_board_state();
  EXPECT_TRUE(state.betting_round == BettingRound::SETUP ||
              state.betting_round != BettingRound::PREFLOP);
}

// ===== Hand Completion Tests =====

TEST_F(PokerBoardComprehensiveTest, AllButOneFoldEndsHand) {
  setupThreePlayerGame();

  auto state = board.get_board_state();

  // Two players fold
  for (int i = 0; i < 2; i++) {
    state = board.get_board_state();
    if (state.betting_round == BettingRound::SETUP)
      break;

    int acting = state.current_player_idx;
    Action fold{static_cast<uint8_t>(acting), PlayerAction::FOLD, 0};
    board.player_act(fold);
  }

  state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::SETUP);
}

TEST_F(PokerBoardComprehensiveTest, LastPlayerWinsPot) {
  setupTwoPlayerGame();

  auto state1 = board.get_board_state();
  int winner = -1;
  int initial_pot = state1.pot;

  // One player folds
  int folder = state1.current_player_idx;
  winner = (folder + 1) % 2;
  int winner_stack_before = state1.players[winner].stack;

  Action fold{static_cast<uint8_t>(folder), PlayerAction::FOLD, 0};
  board.player_act(fold);

  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.betting_round, BettingRound::SETUP);

  // Winner should have gained the pot
  int winner_stack_after = state2.players[winner].stack;
  EXPECT_GT(winner_stack_after, winner_stack_before);
}

// ===== Edge Cases =====

TEST_F(PokerBoardComprehensiveTest, HardResetClearsState) {
  setupTwoPlayerGame();

  auto state1 = board.get_board_state();
  EXPECT_GT(state1.n_players, 0);
  EXPECT_NE(state1.betting_round, BettingRound::SETUP);

  board.hard_reset();

  auto state2 = board.get_board_state();
  EXPECT_EQ(state2.n_players, 0);
  EXPECT_EQ(state2.betting_round, BettingRound::SETUP);
  EXPECT_EQ(state2.pot, 0);
}

TEST_F(PokerBoardComprehensiveTest, CustomBlindAmounts) {
  PokerBoard<6> custom_board(50, 25); // Custom blind amounts
  custom_board.add_player(1000);
  custom_board.add_player(1000);
  custom_board.auto_start();

  auto state = custom_board.get_board_state();
  EXPECT_EQ(state.small_amount, 25);
  EXPECT_EQ(state.big_amount, 50);
  EXPECT_EQ(state.pot, 75);
}

TEST_F(PokerBoardComprehensiveTest, SixPlayerGame) {
  for (int i = 0; i < 6; i++) {
    board.add_player(1000);
  }

  board.auto_start();

  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 6);
  EXPECT_EQ(state.pot, 30);

  // Verify dealer positions
  int dealer = state.dealer_button_idx;
  int sb_idx = (dealer + 1) % 6;
  int bb_idx = (dealer + 2) % 6;
  int utg_idx = (dealer + 3) % 6;

  EXPECT_EQ(state.players[sb_idx].bet, 10);
  EXPECT_EQ(state.players[bb_idx].bet, 20);
  EXPECT_EQ(state.current_player_idx, utg_idx);
}

TEST_F(PokerBoardComprehensiveTest, PlayerActionsAcrossMultipleRounds) {
  setupThreePlayerGame();

  auto state = board.get_board_state();
  int player_idx = 0;
  int initial_stack = state.players[player_idx].stack;

  // Track actions through multiple rounds
  advanceToFlop();
  state = board.get_board_state();

  // Player bets on flop
  if (state.current_player_idx == player_idx) {
    Action bet{static_cast<uint8_t>(player_idx), PlayerAction::BET, 50};
    board.player_act(bet);
  }

  // Advance more
  while (state.betting_round == BettingRound::FLOP) {
    state = board.get_board_state();
    int acting = state.current_player_idx;

    if (state.players[acting].bet < state.current_bet) {
      Action call{static_cast<uint8_t>(acting), PlayerAction::CALL, 0};
      board.player_act(call);
    } else if (acting == player_idx) {
      break;
    } else {
      Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
      board.player_act(check);
    }
  }

  state = board.get_board_state();
  // Verify player has less chips
  EXPECT_LT(state.players[player_idx].stack, initial_stack);
}

TEST_F(PokerBoardComprehensiveTest, DealerButtonWrapsAround) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);

  // Advance dealer through multiple hands
  for (int i = 0; i < 5; i++) {
    board.auto_start();
    auto state = board.get_board_state();
    EXPECT_EQ(state.dealer_button_idx, i % 3);

    // End hand quickly by having players fold
    for (int j = 0; j < 2; j++) {
      state = board.get_board_state();
      if (state.betting_round == BettingRound::SETUP)
        break;
      int acting = state.current_player_idx;
      Action fold{static_cast<uint8_t>(acting), PlayerAction::FOLD, 0};
      board.player_act(fold);
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
