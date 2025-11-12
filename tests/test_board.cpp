#include "../src/Board.cpp"
#include "../src/PokerTypes.hpp"
#include <gtest/gtest.h>

class PokerBoardTest : public ::testing::Test {
protected:
  PokerBoard<6> board;

  void SetUp() override { board.hard_reset(); }
};

// Test basic setup
TEST_F(PokerBoardTest, InitialState) {
  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 0);
  EXPECT_EQ(state.pot, 0);
  EXPECT_EQ(state.betting_round, BettingRound::SETUP);
}

// Test adding players
TEST_F(PokerBoardTest, AddPlayers) {
  int idx1 = board.add_player(1000);
  EXPECT_EQ(idx1, 0);

  int idx2 = board.add_player(1000);
  EXPECT_EQ(idx2, 1);

  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 2);
  EXPECT_EQ(state.players[0].stack, 1000);
  EXPECT_EQ(state.players[1].stack, 1000);
}

// Test adding too many players
TEST_F(PokerBoardTest, AddTooManyPlayers) {
  for (int i = 0; i < 6; i++) {
    board.add_player(1000);
  }

  EXPECT_THROW(board.add_player(1000), std::runtime_error);
}

// Test removing players
TEST_F(PokerBoardTest, RemovePlayer) {
  board.add_player(1000);
  board.add_player(1500);
  board.add_player(2000);

  PlayerState removed = board.remove_player(1);
  EXPECT_EQ(removed.stack, 1500);

  auto state = board.get_board_state();
  EXPECT_EQ(state.n_players, 2);
  EXPECT_EQ(state.players[0].stack, 1000);
  EXPECT_EQ(state.players[1].stack, 2000); // Player 2 moved to index 1
}

// Test posting blinds with 2 players (heads-up)
TEST_F(PokerBoardTest, PostBlindsHeadsUp) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();

  // In heads-up, dealer is small blind
  int small_blind_idx = state.dealer_button_idx;
  int big_blind_idx = (state.dealer_button_idx + 1) % 2;

  EXPECT_EQ(state.players[small_blind_idx].bet, 10);
  EXPECT_EQ(state.players[big_blind_idx].bet, 20);
  EXPECT_EQ(state.pot, 30);
  EXPECT_EQ(state.current_bet, 20);
  EXPECT_EQ(state.current_player_idx,
            small_blind_idx); // Small blind acts first in heads-up
}

// Test posting blinds with 3+ players
TEST_F(PokerBoardTest, PostBlindsMultiplePlayers) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();

  int small_blind_idx = (state.dealer_button_idx + 1) % 3;
  int big_blind_idx = (state.dealer_button_idx + 2) % 3;

  EXPECT_EQ(state.players[small_blind_idx].bet, 10);
  EXPECT_EQ(state.players[big_blind_idx].bet, 20);
  EXPECT_EQ(state.pot, 30);
  EXPECT_EQ(state.current_bet, 20);
  EXPECT_EQ(state.current_player_idx,
            (big_blind_idx + 1) % 3); // UTG acts first
}

// Test player actions - fold
TEST_F(PokerBoardTest, PlayerFold) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  int acting_player = state.current_player_idx;

  Action fold_action{static_cast<uint8_t>(acting_player), PlayerAction::FOLD,
                     0};
  board.player_act(fold_action);

  state = board.get_board_state();
  EXPECT_EQ(state.players[acting_player].state, PlayerState::State::FOLDED);
  EXPECT_EQ(state.betting_round, BettingRound::SETUP); // Hand should end
}

// Test player actions - call
TEST_F(PokerBoardTest, PlayerCall) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  int acting_player = state.current_player_idx;
  int initial_stack = state.players[acting_player].stack;
  int initial_bet = state.players[acting_player].bet;

  Action call_action{static_cast<uint8_t>(acting_player), PlayerAction::CALL,
                     0};
  board.player_act(call_action);

  state = board.get_board_state();
  EXPECT_EQ(state.players[acting_player].bet, 20); // Called the big blind
  EXPECT_EQ(state.players[acting_player].stack,
            initial_stack - (20 - initial_bet));
  EXPECT_GT(state.pot, 30); // At least 10 (SB) + 20 (BB)
}

// Test player actions - raise
TEST_F(PokerBoardTest, PlayerRaise) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  int acting_player = state.current_player_idx;

  Action raise_action{static_cast<uint8_t>(acting_player), PlayerAction::RAISE,
                      50};
  board.player_act(raise_action);

  state = board.get_board_state();
  EXPECT_EQ(state.players[acting_player].bet, 50);
  EXPECT_EQ(state.current_bet, 50);
  EXPECT_EQ(state.last_aggressor_idx, acting_player);
}

// Test player actions - bet
TEST_F(PokerBoardTest, PlayerBet) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();

  // Everyone calls to flop
  for (int i = 0; i < 3; i++) {
    state = board.get_board_state();
    if (state.betting_round != BettingRound::PREFLOP)
      break;

    int acting = state.current_player_idx;
    if (state.players[acting].bet < state.current_bet) {
      Action call{static_cast<uint8_t>(acting), PlayerAction::CALL, 0};
      board.player_act(call);
    } else {
      Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
      board.player_act(check);
    }
  }

  state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::FLOP);

  int acting_player = state.current_player_idx;
  int preflop_bet = state.players[acting_player].bet;
  Action bet_action{static_cast<uint8_t>(acting_player), PlayerAction::BET, 50};
  board.player_act(bet_action);

  state = board.get_board_state();
  EXPECT_EQ(state.players[acting_player].bet,
            preflop_bet + 50); // Total bet includes preflop
  EXPECT_EQ(state.current_bet, preflop_bet + 50);
}

// Test player actions - check
TEST_F(PokerBoardTest, PlayerCheck) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  // Get to flop with everyone calling
  for (int i = 0; i < 3; i++) {
    auto state = board.get_board_state();
    if (state.betting_round != BettingRound::PREFLOP)
      break;

    int acting = state.current_player_idx;
    if (state.players[acting].bet < state.current_bet) {
      Action call{static_cast<uint8_t>(acting), PlayerAction::CALL, 0};
      board.player_act(call);
    } else {
      Action check{static_cast<uint8_t>(acting), PlayerAction::CHECK, 0};
      board.player_act(check);
    }
  }

  auto state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::FLOP);

  int acting_player = state.current_player_idx;
  int initial_bet = state.players[acting_player].bet;

  Action check_action{static_cast<uint8_t>(acting_player), PlayerAction::CHECK,
                      0};
  board.player_act(check_action);

  state = board.get_board_state();
  EXPECT_EQ(state.players[acting_player].bet, initial_bet); // Bet unchanged
}

// Test all-in
TEST_F(PokerBoardTest, PlayerAllIn) {
  board.add_player(200);
  board.add_player(200);
  board.auto_start();

  auto state = board.get_board_state();
  int acting_player = state.current_player_idx;
  int initial_stack = state.players[acting_player].stack;
  int initial_bet = state.players[acting_player].bet;
  int expected_total_bet = initial_stack + initial_bet;

  Action all_in{static_cast<uint8_t>(acting_player), PlayerAction::ALL_IN, 0};
  auto result = board.player_act(all_in);

  state = board.get_board_state();

  // If hand is still in progress, check all-in state
  if (result.has_value()) {
    EXPECT_EQ(state.players[acting_player].stack, 0);
    EXPECT_EQ(state.players[acting_player].bet, expected_total_bet);
    EXPECT_EQ(state.players[acting_player].state, PlayerState::State::ALL_IN);
  } else {
    // Hand ended, just verify the all-in happened
    EXPECT_GT(state.players[acting_player].stack, 0); // Won the pot
  }
}

// Test wrong player acting
TEST_F(PokerBoardTest, WrongPlayerAct) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  int wrong_player = (state.current_player_idx + 1) % 2;

  Action action{static_cast<uint8_t>(wrong_player), PlayerAction::FOLD, 0};
  EXPECT_THROW(board.player_act(action), std::runtime_error);
}

// Test betting round progression
TEST_F(PokerBoardTest, BettingRoundProgression) {
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

  auto state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);

  // Complete preflop betting
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

  state = board.get_board_state();
  EXPECT_EQ(state.betting_round, BettingRound::FLOP);
}

// Test insufficient chips
TEST_F(PokerBoardTest, InsufficientChips) {
  board.add_player(5); // Not enough for small blind (needs 10)
  board.add_player(1000);

  // auto_start should throw when a player doesn't have enough for blinds
  EXPECT_THROW(board.auto_start(), std::runtime_error);
}

// Test hand ending when one player remains
TEST_F(PokerBoardTest, OnePlayerRemaining) {
  board.add_player(1000);
  board.add_player(1000);
  board.add_player(1000);
  board.auto_start();

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
  EXPECT_EQ(state.betting_round, BettingRound::SETUP); // Hand ended
}

// Test stack topup
TEST_F(PokerBoardTest, StackTopup) {
  int idx = board.add_player(1000);
  int new_stack = board.player_stack_topup(idx, 500);

  EXPECT_EQ(new_stack, 1500);

  auto state = board.get_board_state();
  EXPECT_EQ(state.players[idx].stack, 1500);
}

// Test activate/deactivate player
TEST_F(PokerBoardTest, ActivateDeactivatePlayer) {
  int idx = board.add_player(1000);

  board.deactivate_player(idx);
  auto state = board.get_board_state();
  EXPECT_EQ(state.players[idx].state, PlayerState::State::INACTIVE);

  board.activate_player(idx);
  state = board.get_board_state();
  EXPECT_EQ(state.players[idx].state, PlayerState::State::ACTIVE);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
