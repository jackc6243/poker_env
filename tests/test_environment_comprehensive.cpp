#include "../src/PokerEnvironment.cpp"
#include "../src/PokerTypes.hpp"
#include <gtest/gtest.h>

class PokerEnvironmentComprehensiveTest : public ::testing::Test {
protected:
  PokerEnvironment<6> env;

  void SetUp() override { env.reset_game(); }

  // Helper to setup basic 2-player environment
  void setupTwoPlayerEnv() {
    env.upsert_player("alice");
    env.top_up_stack("alice", 1000);
    env.upsert_player("bob");
    env.top_up_stack("bob", 1000);
  }

  // Helper to setup 3-player environment
  void setupThreePlayerEnv() {
    env.upsert_player("alice");
    env.top_up_stack("alice", 1000);
    env.upsert_player("bob");
    env.top_up_stack("bob", 1000);
    env.upsert_player("charlie");
    env.top_up_stack("charlie", 1000);
  }

  // Helper to complete a hand by folding
  void completeHandByFolding() {
    auto state = env.get_board_state();
    while (state.betting_round != BettingRound::SETUP) {
      state = env.step(Action{static_cast<uint8_t>(state.current_player_idx),
                              PlayerAction::FOLD, 0});
    }
  }
};

// ===== Player Management Tests =====

TEST_F(PokerEnvironmentComprehensiveTest, UpsertSinglePlayer) {
  EXPECT_NO_THROW(env.upsert_player("alice"));

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.size(), 1);
  EXPECT_TRUE(player_map.at("alice").active);
  EXPECT_EQ(player_map.at("alice").idx, -1); // Not yet in board
}

TEST_F(PokerEnvironmentComprehensiveTest, UpsertMultiplePlayers) {
  env.upsert_player("alice");
  env.upsert_player("bob");
  env.upsert_player("charlie");

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.size(), 3);
  EXPECT_TRUE(player_map.at("alice").active);
  EXPECT_TRUE(player_map.at("bob").active);
  EXPECT_TRUE(player_map.at("charlie").active);
}

TEST_F(PokerEnvironmentComprehensiveTest, UpsertMaxPlayers) {
  for (int i = 0; i < 6; i++) {
    EXPECT_NO_THROW(env.upsert_player("player" + std::to_string(i)));
  }

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.size(), 6);
}

TEST_F(PokerEnvironmentComprehensiveTest, UpsertBeyondMaxPlayers) {
  for (int i = 0; i < 6; i++) {
    env.upsert_player("player" + std::to_string(i));
  }

  EXPECT_THROW(env.upsert_player("extra_player"), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, UpsertReactivatesInactivePlayer) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);
  env.remove_player("alice");

  auto player_map = env.get_player_map();
  EXPECT_FALSE(player_map.at("alice").active);

  env.upsert_player("alice");
  player_map = env.get_player_map();
  EXPECT_TRUE(player_map.at("alice").active);
}

TEST_F(PokerEnvironmentComprehensiveTest, RemoveActivePlayer) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);

  EXPECT_NO_THROW(env.remove_player("alice"));

  auto player_map = env.get_player_map();
  EXPECT_FALSE(player_map.at("alice").active);
}

TEST_F(PokerEnvironmentComprehensiveTest, RemoveNonExistentPlayer) {
  EXPECT_THROW(env.remove_player("ghost"), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, RemovePlayerNotYetInBoard) {
  env.upsert_player("alice");

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.at("alice").idx, -1);

  // Should erase completely since idx is -1
  env.remove_player("alice");

  player_map = env.get_player_map();
  EXPECT_EQ(player_map.find("alice"), player_map.end());
}

TEST_F(PokerEnvironmentComprehensiveTest, RemovePlayerInBoard) {
  setupTwoPlayerEnv();
  auto [state, left] = env.reset_hand();

  // Complete the hand
  completeHandByFolding();

  // Now remove a player
  env.remove_player("alice");

  auto player_map = env.get_player_map();
  EXPECT_FALSE(player_map.at("alice").active);
  EXPECT_NE(player_map.at("alice").idx, -1); // Still has index until next reset
}

// ===== Stack Management Tests =====

TEST_F(PokerEnvironmentComprehensiveTest, TopUpNewPlayer) {
  env.upsert_player("alice");

  EXPECT_NO_THROW(env.top_up_stack("alice", 500));

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.at("alice").stack_topup, 500);
}

TEST_F(PokerEnvironmentComprehensiveTest, TopUpMultipleTimes) {
  env.upsert_player("alice");

  env.top_up_stack("alice", 500);
  env.top_up_stack("alice", 300);
  env.top_up_stack("alice", 200);

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.at("alice").stack_topup, 1000);
}

TEST_F(PokerEnvironmentComprehensiveTest, TopUpNonExistentPlayer) {
  EXPECT_THROW(env.top_up_stack("ghost", 1000), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, TopUpInactivePlayer) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);
  env.remove_player("alice");

  EXPECT_THROW(env.top_up_stack("alice", 500), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, TopUpBetweenHands) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  // Complete hand
  completeHandByFolding();

  // Top up a player
  env.top_up_stack("alice", 500);

  auto [state2, left2] = env.reset_hand();

  // Find alice's index in the new state
  auto player_map = env.get_player_map();
  int alice_idx = player_map.at("alice").idx;

  // Stack should reflect the top-up (minus blinds)
  EXPECT_GT(state2.players[alice_idx].stack + state2.players[alice_idx].bet,
            1000);
}

TEST_F(PokerEnvironmentComprehensiveTest, StackTopupClearedAfterReset) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);

  auto player_map1 = env.get_player_map();
  EXPECT_EQ(player_map1.at("alice").stack_topup, 1000);

  env.upsert_player("bob");
  env.top_up_stack("bob", 1000);

  env.reset_hand();

  auto player_map2 = env.get_player_map();
  EXPECT_EQ(player_map2.at("alice").stack_topup, 0);
  EXPECT_EQ(player_map2.at("bob").stack_topup, 0);
}

// ===== Game Reset Tests =====

TEST_F(PokerEnvironmentComprehensiveTest, ResetGameClearsPlayers) {
  setupThreePlayerEnv();

  auto player_map1 = env.get_player_map();
  EXPECT_EQ(player_map1.size(), 3);

  env.reset_game();

  auto player_map2 = env.get_player_map();
  EXPECT_EQ(player_map2.size(), 0);
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetGameClearsBoard) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  EXPECT_GT(state1.n_players, 0);

  env.reset_game();

  // After reset, should be able to add players again
  EXPECT_NO_THROW(env.upsert_player("alice"));
  EXPECT_NO_THROW(env.top_up_stack("alice", 1000));
}

TEST_F(PokerEnvironmentComprehensiveTest, MultipleResetGames) {
  for (int i = 0; i < 3; i++) {
    setupTwoPlayerEnv();
    auto [state, left] = env.reset_hand();
    EXPECT_EQ(state.n_players, 2);

    completeHandByFolding();
    env.reset_game();
  }

  auto player_map = env.get_player_map();
  EXPECT_EQ(player_map.size(), 0);
}

// ===== Hand Reset Tests =====

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandNotEnoughPlayers) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);

  EXPECT_THROW(env.reset_hand(), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandWithTwoPlayers) {
  setupTwoPlayerEnv();

  auto [state, left] = env.reset_hand();

  EXPECT_EQ(state.n_players, 2);
  EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);
  EXPECT_EQ(state.pot, 30); // Small blind + big blind
  EXPECT_TRUE(left.empty());
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandWithThreePlayers) {
  setupThreePlayerEnv();

  auto [state, left] = env.reset_hand();

  EXPECT_EQ(state.n_players, 3);
  EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);
  EXPECT_EQ(state.pot, 30);
  EXPECT_TRUE(left.empty());
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandWhileHandInProgress) {
  setupTwoPlayerEnv();
  env.reset_hand();

  EXPECT_THROW(env.reset_hand(), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandRemovesInactivePlayers) {
  setupThreePlayerEnv();
  auto [state1, left1] = env.reset_hand();
  EXPECT_EQ(state1.n_players, 3);

  completeHandByFolding();

  env.remove_player("charlie");

  auto [state2, left2] = env.reset_hand();

  EXPECT_EQ(state2.n_players, 2);
  EXPECT_EQ(left2.size(), 1);
  EXPECT_EQ(left2[0].first, "charlie");
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandAddsNewPlayers) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();
  EXPECT_EQ(state1.n_players, 2);

  completeHandByFolding();

  env.upsert_player("charlie");
  env.top_up_stack("charlie", 1500);

  auto [state2, left2] = env.reset_hand();

  EXPECT_EQ(state2.n_players, 3);
  EXPECT_TRUE(left2.empty());
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandPlayerIndexMaintained) {
  setupThreePlayerEnv();
  auto [state1, left1] = env.reset_hand();

  auto player_map1 = env.get_player_map();
  int alice_idx1 = player_map1.at("alice").idx;
  int bob_idx1 = player_map1.at("bob").idx;
  int charlie_idx1 = player_map1.at("charlie").idx;

  completeHandByFolding();

  auto [state2, left2] = env.reset_hand();

  auto player_map2 = env.get_player_map();
  EXPECT_EQ(player_map2.at("alice").idx, alice_idx1);
  EXPECT_EQ(player_map2.at("bob").idx, bob_idx1);
  EXPECT_EQ(player_map2.at("charlie").idx, charlie_idx1);
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandIndexAdjustedAfterRemoval) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);
  env.upsert_player("bob");
  env.top_up_stack("bob", 1000);
  env.upsert_player("charlie");
  env.top_up_stack("charlie", 1000);

  auto [state1, left1] = env.reset_hand();

  auto player_map1 = env.get_player_map();
  int alice_idx = player_map1.at("alice").idx;
  int bob_idx = player_map1.at("bob").idx;
  int charlie_idx = player_map1.at("charlie").idx;

  completeHandByFolding();

  // Remove the middle player
  env.remove_player("bob");

  auto [state2, left2] = env.reset_hand();

  EXPECT_EQ(state2.n_players, 2);

  auto player_map2 = env.get_player_map();

  // If alice was first and bob was removed, alice stays at same index
  // If charlie was after bob, charlie's index should decrease
  if (bob_idx < charlie_idx) {
    EXPECT_EQ(player_map2.at("charlie").idx, charlie_idx - 1);
  }
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandMultipleInactivePlayers) {
  for (int i = 0; i < 5; i++) {
    env.upsert_player("player" + std::to_string(i));
    env.top_up_stack("player" + std::to_string(i), 1000);
  }

  auto [state1, left1] = env.reset_hand();
  EXPECT_EQ(state1.n_players, 5);

  completeHandByFolding();

  // Remove multiple players
  env.remove_player("player1");
  env.remove_player("player3");

  auto [state2, left2] = env.reset_hand();

  EXPECT_EQ(state2.n_players, 3);
  EXPECT_EQ(left2.size(), 2);

  // Check that the left players are correct
  bool found_player1 = false, found_player3 = false;
  for (const auto &[name, stack] : left2) {
    if (name == "player1")
      found_player1 = true;
    if (name == "player3")
      found_player3 = true;
  }
  EXPECT_TRUE(found_player1);
  EXPECT_TRUE(found_player3);
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandAppliesTopups) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  auto player_map1 = env.get_player_map();
  int alice_idx = player_map1.at("alice").idx;
  int alice_initial_total =
      state1.players[alice_idx].stack + state1.players[alice_idx].bet;

  completeHandByFolding();

  env.top_up_stack("alice", 500);

  auto [state2, left2] = env.reset_hand();

  auto player_map2 = env.get_player_map();
  alice_idx = player_map2.at("alice").idx;
  int alice_new_total =
      state2.players[alice_idx].stack + state2.players[alice_idx].bet;

  EXPECT_GT(alice_new_total, alice_initial_total);
}

TEST_F(PokerEnvironmentComprehensiveTest, ResetHandReturnsLeftPlayerStacks) {
  setupThreePlayerEnv();
  auto [state1, left1] = env.reset_hand();

  auto player_map1 = env.get_player_map();
  int charlie_idx = player_map1.at("charlie").idx;

  completeHandByFolding();

  // Record Charlie's stack before removal
  auto player_map2 = env.get_player_map();

  env.remove_player("charlie");

  auto [state2, left2] = env.reset_hand();

  ASSERT_EQ(left2.size(), 1);
  EXPECT_EQ(left2[0].first, "charlie");
  EXPECT_GT(left2[0].second, 0); // Should have some stack
}

// ===== Step Function Tests =====

TEST_F(PokerEnvironmentComprehensiveTest, StepWithoutHandInProgress) {
  setupTwoPlayerEnv();

  Action action{0, PlayerAction::FOLD, 0};
  EXPECT_THROW(env.step(action), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, StepValidFold) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  int acting_player = state1.current_player_idx;
  Action fold{static_cast<uint8_t>(acting_player), PlayerAction::FOLD, 0};

  auto state2 = env.step(fold);

  EXPECT_EQ(state2.players[acting_player].state, PlayerState::State::FOLDED);
  EXPECT_EQ(state2.betting_round, BettingRound::SETUP); // Hand ends
}

TEST_F(PokerEnvironmentComprehensiveTest, StepValidCall) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  int acting_player = state1.current_player_idx;
  int initial_bet = state1.players[acting_player].bet;

  Action call{static_cast<uint8_t>(acting_player), PlayerAction::CALL, 0};
  auto state2 = env.step(call);

  EXPECT_EQ(state2.players[acting_player].bet, 20); // Called to big blind
}

TEST_F(PokerEnvironmentComprehensiveTest, StepValidRaise) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  int acting_player = state1.current_player_idx;

  Action raise{static_cast<uint8_t>(acting_player), PlayerAction::RAISE, 60};
  auto state2 = env.step(raise);

  EXPECT_EQ(state2.current_bet, 60);
  EXPECT_EQ(state2.players[acting_player].bet, 60);
}

TEST_F(PokerEnvironmentComprehensiveTest, StepInvalidAction) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  int acting_player = state1.current_player_idx;

  // Try to check when facing a bet
  Action check{static_cast<uint8_t>(acting_player), PlayerAction::CHECK, 0};
  EXPECT_THROW(env.step(check), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, StepWrongPlayer) {
  setupTwoPlayerEnv();
  auto [state1, left1] = env.reset_hand();

  int wrong_player = (state1.current_player_idx + 1) % 2;

  Action action{static_cast<uint8_t>(wrong_player), PlayerAction::FOLD, 0};
  EXPECT_THROW(env.step(action), std::runtime_error);
}

TEST_F(PokerEnvironmentComprehensiveTest, StepSequenceThroughHand) {
  setupTwoPlayerEnv();
  auto [state, left] = env.reset_hand();

  // Player 1 calls
  state = env.step(Action{static_cast<uint8_t>(state.current_player_idx),
                          PlayerAction::CALL, 0});

  // Player 2 checks
  state = env.step(Action{static_cast<uint8_t>(state.current_player_idx),
                          PlayerAction::CHECK, 0});

  EXPECT_EQ(state.betting_round, BettingRound::FLOP);

  // Both check on flop
  state = env.step(Action{static_cast<uint8_t>(state.current_player_idx),
                          PlayerAction::CHECK, 0});
  state = env.step(Action{static_cast<uint8_t>(state.current_player_idx),
                          PlayerAction::CHECK, 0});

  EXPECT_EQ(state.betting_round, BettingRound::TURN);
}

TEST_F(PokerEnvironmentComprehensiveTest, StepAllIn) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 100);
  env.upsert_player("bob");
  env.top_up_stack("bob", 1000);

  auto [state1, left1] = env.reset_hand();

  // Find the short stack player
  int short_stack = -1;
  for (int i = 0; i < 2; i++) {
    if (state1.players[i].stack + state1.players[i].bet <= 100) {
      short_stack = i;
      break;
    }
  }

  if (short_stack != -1 && state1.current_player_idx == short_stack) {
    Action all_in{static_cast<uint8_t>(short_stack), PlayerAction::ALL_IN, 0};
    auto state2 = env.step(all_in);

    if (state2.betting_round != BettingRound::SETUP) {
      EXPECT_EQ(state2.players[short_stack].state, PlayerState::State::ALL_IN);
      EXPECT_EQ(state2.players[short_stack].stack, 0);
    }
  }
}

// ===== Complex Scenarios =====

TEST_F(PokerEnvironmentComprehensiveTest, MultipleHandsSequence) {
  setupTwoPlayerEnv();

  for (int hand = 0; hand < 3; hand++) {
    auto [state, left] = env.reset_hand();
    EXPECT_EQ(state.n_players, 2);
    EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);

    // Complete hand by folding
    completeHandByFolding();
  }
}

TEST_F(PokerEnvironmentComprehensiveTest, PlayerJoinsAndLeavesBetweenHands) {
  setupTwoPlayerEnv();

  // Hand 1
  auto [state1, left1] = env.reset_hand();
  EXPECT_EQ(state1.n_players, 2);
  completeHandByFolding();

  // Add player
  env.upsert_player("charlie");
  env.top_up_stack("charlie", 1500);

  // Hand 2
  auto [state2, left2] = env.reset_hand();
  EXPECT_EQ(state2.n_players, 3);
  completeHandByFolding();

  // Remove player
  env.remove_player("charlie");

  // Hand 3
  auto [state3, left3] = env.reset_hand();
  EXPECT_EQ(state3.n_players, 2);
  EXPECT_EQ(left3.size(), 1);
  EXPECT_EQ(left3[0].first, "charlie");
}

TEST_F(PokerEnvironmentComprehensiveTest, PlayersBustAndTopUp) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 50); // Low stack
  env.upsert_player("bob");
  env.top_up_stack("bob", 1000);

  auto [state1, left1] = env.reset_hand();

  // Play out hand - alice might bust
  completeHandByFolding();

  // Top up alice
  env.top_up_stack("alice", 1000);

  auto [state2, left2] = env.reset_hand();

  auto player_map = env.get_player_map();
  int alice_idx = player_map.at("alice").idx;

  // Alice should have chips again
  EXPECT_GT(state2.players[alice_idx].stack + state2.players[alice_idx].bet,
            100);
}

TEST_F(PokerEnvironmentComprehensiveTest, AllPlayersRemoveAndReadd) {
  setupThreePlayerEnv();
  auto [state1, left1] = env.reset_hand();

  completeHandByFolding();

  // Remove all
  env.remove_player("alice");
  env.remove_player("bob");
  env.remove_player("charlie");

  // Try to start hand - should fail
  EXPECT_THROW(env.reset_hand(), std::runtime_error);

  // Reactivate some
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);
  env.upsert_player("bob");
  env.top_up_stack("bob", 1000);

  auto [state2, left2] = env.reset_hand();
  EXPECT_EQ(state2.n_players, 2);
}

TEST_F(PokerEnvironmentComprehensiveTest, ComplexResetHandScenario) {
  // Start with 4 players
  for (int i = 0; i < 4; i++) {
    env.upsert_player("player" + std::to_string(i));
    env.top_up_stack("player" + std::to_string(i), 1000);
  }

  auto [state1, left1] = env.reset_hand();
  EXPECT_EQ(state1.n_players, 4);

  completeHandByFolding();

  // Remove 1 player
  env.remove_player("player1");

  // Add 1 new player
  env.upsert_player("player4");
  env.top_up_stack("player4", 1500);

  // Top up existing player
  env.top_up_stack("player0", 500);

  auto [state2, left2] = env.reset_hand();

  EXPECT_EQ(state2.n_players, 4);
  EXPECT_EQ(left2.size(), 1);
  EXPECT_EQ(left2[0].first, "player1");

  // Verify player4 has correct stack
  auto player_map = env.get_player_map();
  int player4_idx = player_map.at("player4").idx;
  EXPECT_GT(state2.players[player4_idx].stack + state2.players[player4_idx].bet,
            1400);
}

TEST_F(PokerEnvironmentComprehensiveTest, DealerButtonProgression) {
  setupThreePlayerEnv();

  std::vector<int> dealer_positions;

  for (int hand = 0; hand < 5; hand++) {
    auto [state, left] = env.reset_hand();
    dealer_positions.push_back(state.dealer_button_idx);

    completeHandByFolding();
  }

  // Verify dealer button advances
  for (size_t i = 1; i < dealer_positions.size(); i++) {
    int expected = (dealer_positions[i - 1] + 1) % 3;
    EXPECT_EQ(dealer_positions[i], expected);
  }
}

TEST_F(PokerEnvironmentComprehensiveTest, StackConsistencyAcrossHands) {
  setupTwoPlayerEnv();

  auto [state1, left1] = env.reset_hand();

  auto player_map1 = env.get_player_map();
  int alice_idx = player_map1.at("alice").idx;
  int bob_idx = player_map1.at("bob").idx;

  int total_chips_before = 0;
  for (int i = 0; i < 2; i++) {
    total_chips_before += state1.players[i].stack + state1.players[i].bet;
  }

  // Play some actions
  auto state2 = env.step(Action{static_cast<uint8_t>(state1.current_player_idx),
                                PlayerAction::CALL, 0});
  state2 = env.step(Action{static_cast<uint8_t>(state2.current_player_idx),
                           PlayerAction::CHECK, 0});

  // Someone bets on flop
  state2 = env.step(Action{static_cast<uint8_t>(state2.current_player_idx),
                           PlayerAction::BET, 50});

  // Other folds
  state2 = env.step(Action{static_cast<uint8_t>(state2.current_player_idx),
                           PlayerAction::FOLD, 0});

  EXPECT_EQ(state2.betting_round, BettingRound::SETUP);

  // Check total chips is still the same
  int total_chips_after = 0;
  for (int i = 0; i < 2; i++) {
    total_chips_after += state2.players[i].stack;
  }

  EXPECT_EQ(total_chips_before, total_chips_after);
}

TEST_F(PokerEnvironmentComprehensiveTest, SixPlayerGameFlow) {
  for (int i = 0; i < 6; i++) {
    env.upsert_player("player" + std::to_string(i));
    env.top_up_stack("player" + std::to_string(i), 1000);
  }

  auto [state, left] = env.reset_hand();

  EXPECT_EQ(state.n_players, 6);
  EXPECT_EQ(state.pot, 30);
  EXPECT_EQ(state.betting_round, BettingRound::PREFLOP);

  // Verify blinds are correct
  int dealer = state.dealer_button_idx;
  int sb_idx = (dealer + 1) % 6;
  int bb_idx = (dealer + 2) % 6;

  EXPECT_EQ(state.players[sb_idx].bet, 10);
  EXPECT_EQ(state.players[bb_idx].bet, 20);
}

TEST_F(PokerEnvironmentComprehensiveTest, PlayerNamePersistence) {
  env.upsert_player("alice");
  env.top_up_stack("alice", 1000);
  env.upsert_player("bob");
  env.top_up_stack("bob", 1000);

  auto [state1, left1] = env.reset_hand();

  auto player_map1 = env.get_player_map();
  int alice_idx1 = player_map1.at("alice").idx;
  int bob_idx1 = player_map1.at("bob").idx;

  completeHandByFolding();

  auto [state2, left2] = env.reset_hand();

  auto player_map2 = env.get_player_map();

  // Player names should still map to same indices
  EXPECT_EQ(player_map2.at("alice").idx, alice_idx1);
  EXPECT_EQ(player_map2.at("bob").idx, bob_idx1);
}

TEST_F(PokerEnvironmentComprehensiveTest, EmptyLeftPlayersWhenNoRemoval) {
  setupThreePlayerEnv();

  auto [state1, left1] = env.reset_hand();
  EXPECT_TRUE(left1.empty());

  completeHandByFolding();

  auto [state2, left2] = env.reset_hand();
  EXPECT_TRUE(left2.empty());
}

TEST_F(PokerEnvironmentComprehensiveTest, PlayerStateConsistency) {
  setupTwoPlayerEnv();

  auto player_map1 = env.get_player_map();

  // All players should be active and not yet in board
  for (const auto &[name, info] : player_map1) {
    EXPECT_TRUE(info.active);
  }

  auto [state, left] = env.reset_hand();

  auto player_map2 = env.get_player_map();

  // All players should now have indices
  for (const auto &[name, info] : player_map2) {
    EXPECT_NE(info.idx, -1);
    EXPECT_EQ(info.stack_topup, 0); // Should be cleared
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
