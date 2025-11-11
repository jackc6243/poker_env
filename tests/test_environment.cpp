#include "../src/PokerEnvironment.cpp"
#include "../src/PokerTypes.hpp"
#include <gtest/gtest.h>

class PokerEnvironmentTest : public ::testing::Test {
protected:
  PokerEnvironment<6> env;

  void SetUp() override { env.reset_game(); }
};

// Test upserting players
TEST_F(PokerEnvironmentTest, UpsertNewPlayer) {
  EXPECT_NO_THROW(env.upsert_player("player1"));
  EXPECT_NO_THROW(env.upsert_player("player2"));
}

// Test upserting existing player
TEST_F(PokerEnvironmentTest, UpsertExistingPlayer) {
  env.upsert_player("player1");
  env.remove_player("player1");

  // Upsert should reactivate the player
  EXPECT_NO_THROW(env.upsert_player("player1"));
}

// Test max players limit
TEST_F(PokerEnvironmentTest, MaxPlayersLimit) {
  for (int i = 0; i < 6; i++) {
    env.upsert_player("player" + std::to_string(i));
  }

  EXPECT_THROW(env.upsert_player("player7"), std::runtime_error);
}

// Test topping up stack
TEST_F(PokerEnvironmentTest, TopUpStack) {
  env.upsert_player("player1");
  EXPECT_NO_THROW(env.top_up_stack("player1", 1000));
  EXPECT_NO_THROW(env.top_up_stack("player1", 500));
}

// Test topping up non-existent player
TEST_F(PokerEnvironmentTest, TopUpNonExistentPlayer) {
  EXPECT_THROW(env.top_up_stack("ghost", 1000), std::runtime_error);
}

// Test topping up inactive player
TEST_F(PokerEnvironmentTest, TopUpInactivePlayer) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.remove_player("player1");

  EXPECT_THROW(env.top_up_stack("player1", 500), std::runtime_error);
}

// Test removing player
TEST_F(PokerEnvironmentTest, RemovePlayer) {
  env.upsert_player("player1");
  EXPECT_NO_THROW(env.remove_player("player1"));
}

// Test removing non-existent player
TEST_F(PokerEnvironmentTest, RemoveNonExistentPlayer) {
  EXPECT_THROW(env.remove_player("ghost"), std::runtime_error);
}

// Test reset game
TEST_F(PokerEnvironmentTest, ResetGame) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);

  EXPECT_NO_THROW(env.reset_game());
}

// Test reset hand with not enough players
TEST_F(PokerEnvironmentTest, ResetHandNotEnoughPlayers) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);

  EXPECT_THROW(env.reset_hand(), std::runtime_error);
}

// Test reset hand with enough players
TEST_F(PokerEnvironmentTest, ResetHandSuccess) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  auto [board_state, left_players] = env.reset_hand();

  EXPECT_EQ(board_state.n_players, 2);
  EXPECT_EQ(board_state.betting_round, BettingRound::PREFLOP);
  EXPECT_TRUE(left_players.empty());
}

// Test reset hand with inactive player
TEST_F(PokerEnvironmentTest, ResetHandWithInactivePlayer) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);
  env.upsert_player("player3");
  env.top_up_stack("player3", 500);

  // Start a hand first
  auto [state1, left1] = env.reset_hand();

  // Finish the hand by having all but one fold
  state1 = env.step(Action{static_cast<uint8_t>(state1.current_player_idx),
                           PlayerAction::FOLD, 0});
  state1 = env.step(Action{static_cast<uint8_t>(state1.current_player_idx),
                           PlayerAction::FOLD, 0});

  // Remove a player
  env.remove_player("player3");

  // Reset hand should remove the inactive player
  auto [state2, left2] = env.reset_hand();

  EXPECT_EQ(state2.n_players, 2);
  EXPECT_EQ(left2.size(), 1);
  EXPECT_EQ(left2[0].first, "player3");
}

// Test step with valid action
TEST_F(PokerEnvironmentTest, StepValidAction) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  auto [board_state, _] = env.reset_hand();

  Action call_action{static_cast<uint8_t>(board_state.current_player_idx),
                     PlayerAction::CALL, 0};
  auto new_state = env.step(call_action);

  // In heads-up, after call and check/fold, betting round can advance
  EXPECT_TRUE(new_state.betting_round == BettingRound::PREFLOP ||
              new_state.betting_round == BettingRound::FLOP);
}

// Test step without hand in progress
TEST_F(PokerEnvironmentTest, StepNoHandInProgress) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  Action action{0, PlayerAction::FOLD, 0};
  EXPECT_THROW(env.step(action), std::runtime_error);
}

// Test adding new players between hands
TEST_F(PokerEnvironmentTest, AddPlayerBetweenHands) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  auto [state1, _] = env.reset_hand();

  // Finish the hand
  env.step(Action{static_cast<uint8_t>(state1.current_player_idx),
                  PlayerAction::FOLD, 0});

  // Add a new player
  env.upsert_player("player3");
  env.top_up_stack("player3", 1000);

  // Reset hand should include new player
  auto [state2, left2] = env.reset_hand();
  EXPECT_EQ(state2.n_players, 3);
}

// Test player stack management across hands
TEST_F(PokerEnvironmentTest, StackManagementAcrossHands) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  auto [state1, _] = env.reset_hand();

  // Player folds, losing their blind
  int folding_player = state1.current_player_idx;
  env.step(Action{static_cast<uint8_t>(folding_player), PlayerAction::FOLD, 0});

  // Top up the folding player
  std::string player_id = (folding_player == 0) ? "player1" : "player2";
  env.top_up_stack(player_id, 100);

  // Start new hand
  auto [state2, left2] = env.reset_hand();

  // Check that topup was applied
  // Note: exact stack depends on which player folded and what blinds they
  // posted
  EXPECT_GT(state2.players[folding_player].stack, 900); // Should have topped up
}

// Test complete hand flow
TEST_F(PokerEnvironmentTest, CompleteHandFlow) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  auto [state, _] = env.reset_hand();

  EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);
  EXPECT_EQ(state.pot, 30); // Small blind + big blind

  // First player acts
  int player1_idx = state.current_player_idx;
  auto state2 = env.step(
      Action{static_cast<uint8_t>(player1_idx), PlayerAction::CALL, 0});

  // Second player can check (already in for big blind) or fold
  int player2_idx = state2.current_player_idx;
  auto state3 = env.step(
      Action{static_cast<uint8_t>(player2_idx), PlayerAction::CHECK, 0});

  // Should advance to flop
  EXPECT_EQ(state3.betting_round, BettingRound::FLOP);
}

// Test player ordering maintained
TEST_F(PokerEnvironmentTest, PlayerOrderingMaintained) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);
  env.upsert_player("player3");
  env.top_up_stack("player3", 1000);

  auto [state1, _] = env.reset_hand();
  int dealer1 = state1.dealer_button_idx;

  // Complete hand - need 2 players to fold in 3-player game
  state1 = env.step(Action{static_cast<uint8_t>(state1.current_player_idx),
                           PlayerAction::FOLD, 0});
  state1 = env.step(Action{static_cast<uint8_t>(state1.current_player_idx),
                           PlayerAction::FOLD, 0});

  // Start new hand
  auto [state2, left2] = env.reset_hand();
  int dealer2 = state2.dealer_button_idx;

  // Dealer button should advance
  EXPECT_EQ((dealer1 + 1) % 3, dealer2);
}

// Test resetting hand while hand in progress
TEST_F(PokerEnvironmentTest, ResetHandWhileInProgress) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 1000);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);

  env.reset_hand();

  EXPECT_THROW(env.reset_hand(), std::runtime_error);
}

// Test multiple players with different stacks
TEST_F(PokerEnvironmentTest, MultiplePlayersVariousStacks) {
  env.upsert_player("player1");
  env.top_up_stack("player1", 500);
  env.upsert_player("player2");
  env.top_up_stack("player2", 1000);
  env.upsert_player("player3");
  env.top_up_stack("player3", 1500);

  auto [state, _] = env.reset_hand();

  EXPECT_EQ(state.n_players, 3);
  // Verify stacks are set correctly (minus blinds for those who posted)
  int total_stack = 0;
  for (int i = 0; i < 3; i++) {
    total_stack += state.players[i].stack + state.players[i].bet;
  }
  EXPECT_EQ(total_stack, 3000); // 500 + 1000 + 1500
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
