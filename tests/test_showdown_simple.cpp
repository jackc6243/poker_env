#include "../src/Board.cpp"
#include "../src/PokerTypes.hpp"
#include <gtest/gtest.h>

// Direct tests of the showdown implementation functions

// Helper to create a card from suite and number
Card make_card_helper(int suite, int number) {
  Card c;
  c.suite = suite;
  c.number = number;
  return c;
}

// Test hand evaluation directly
TEST(HandEvaluationTest, RoyalFlush) {
  std::array<Card, 2> hole = {make_card_helper(2, 1),
                              make_card_helper(2, 13)}; // A and K of hearts
  std::array<Card, 5> community = {
      make_card_helper(2, 10), make_card_helper(2, 11), make_card_helper(2, 12),
      make_card_helper(0, 7), make_card_helper(1, 8)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank),
            static_cast<int>(HandRank::ROYAL_FLUSH));
}

TEST(HandEvaluationTest, StraightFlush) {
  std::array<Card, 2> hole = {make_card_helper(3, 5),
                              make_card_helper(3, 6)}; // 5-6 of spades
  std::array<Card, 5> community = {
      make_card_helper(3, 7), make_card_helper(3, 8), make_card_helper(3, 9),
      make_card_helper(0, 2), make_card_helper(1, 3)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank),
            static_cast<int>(HandRank::STRAIGHT_FLUSH));
}

TEST(HandEvaluationTest, FourOfAKind) {
  std::array<Card, 2> hole = {make_card_helper(0, 13),
                              make_card_helper(1, 13)}; // Two Kings
  std::array<Card, 5> community = {
      make_card_helper(2, 13), make_card_helper(3, 13), make_card_helper(2, 10),
      make_card_helper(0, 5), make_card_helper(1, 7)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank),
            static_cast<int>(HandRank::FOUR_OF_A_KIND));
}

TEST(HandEvaluationTest, FullHouse) {
  std::array<Card, 2> hole = {make_card_helper(0, 1),
                              make_card_helper(1, 1)}; // Pair of Aces
  std::array<Card, 5> community = {
      make_card_helper(2, 1), make_card_helper(2, 13), make_card_helper(3, 13),
      make_card_helper(0, 2), make_card_helper(1, 3)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank),
            static_cast<int>(HandRank::FULL_HOUSE));
  EXPECT_EQ(hand.kickers[0], 14); // Ace (converted to 14)
}

TEST(HandEvaluationTest, Flush) {
  std::array<Card, 2> hole = {make_card_helper(2, 1),
                              make_card_helper(2, 9)}; // A and 9 of hearts
  std::array<Card, 5> community = {
      make_card_helper(2, 2), make_card_helper(2, 4), make_card_helper(2, 6),
      make_card_helper(0, 10), make_card_helper(1, 11)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank), static_cast<int>(HandRank::FLUSH));
  EXPECT_EQ(hand.kickers[0], 14); // Ace high
}

TEST(HandEvaluationTest, Straight) {
  std::array<Card, 2> hole = {make_card_helper(0, 9),
                              make_card_helper(1, 13)}; // 9 and K
  std::array<Card, 5> community = {
      make_card_helper(2, 10), make_card_helper(3, 11), make_card_helper(0, 12),
      make_card_helper(1, 2), make_card_helper(2, 3)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank), static_cast<int>(HandRank::STRAIGHT));
  EXPECT_EQ(hand.kickers[0], 13); // King high straight
}

TEST(HandEvaluationTest, WheelStraight) {
  std::array<Card, 2> hole = {make_card_helper(0, 1),
                              make_card_helper(1, 2)}; // A and 2
  std::array<Card, 5> community = {
      make_card_helper(2, 3), make_card_helper(3, 4), make_card_helper(0, 5),
      make_card_helper(1, 9), make_card_helper(2, 7)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank), static_cast<int>(HandRank::STRAIGHT));
  EXPECT_EQ(hand.kickers[0], 5); // Wheel is 5-high
}

TEST(HandEvaluationTest, ThreeOfAKind) {
  std::array<Card, 2> hole = {make_card_helper(0, 1),
                              make_card_helper(1, 1)}; // Pair of Aces
  std::array<Card, 5> community = {
      make_card_helper(2, 1), make_card_helper(2, 13), make_card_helper(3, 5),
      make_card_helper(0, 7), make_card_helper(1, 9)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank),
            static_cast<int>(HandRank::THREE_OF_A_KIND));
  EXPECT_EQ(hand.kickers[0], 14); // Three Aces
}

TEST(HandEvaluationTest, TwoPair) {
  std::array<Card, 2> hole = {make_card_helper(0, 1),
                              make_card_helper(1, 13)}; // A and K
  std::array<Card, 5> community = {
      make_card_helper(2, 1), make_card_helper(2, 13), make_card_helper(3, 12),
      make_card_helper(0, 5), make_card_helper(1, 7)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank), static_cast<int>(HandRank::TWO_PAIR));
  EXPECT_EQ(hand.kickers[0], 14); // Aces
  EXPECT_EQ(hand.kickers[1], 13); // Kings
}

TEST(HandEvaluationTest, OnePair) {
  std::array<Card, 2> hole = {make_card_helper(0, 1),
                              make_card_helper(1, 13)}; // A and K
  std::array<Card, 5> community = {
      make_card_helper(3, 1), make_card_helper(2, 7), make_card_helper(0, 9),
      make_card_helper(1, 2), make_card_helper(0, 4)};

  auto hand = evaluate_hand(hole, community);
  EXPECT_EQ(static_cast<int>(hand.rank), static_cast<int>(HandRank::ONE_PAIR));
  EXPECT_EQ(hand.kickers[0], 14); // Pair of Aces
}

TEST(HandEvaluationTest, HandComparison) {
  // Player 0: Aces
  std::array<Card, 2> hole0 = {make_card_helper(0, 1), make_card_helper(1, 1)};
  // Player 1: Kings
  std::array<Card, 2> hole1 = {make_card_helper(0, 13),
                               make_card_helper(1, 13)};

  std::array<Card, 5> community = {
      make_card_helper(2, 2), make_card_helper(3, 4), make_card_helper(0, 6),
      make_card_helper(1, 7), make_card_helper(2, 8)};

  auto hand0 = evaluate_hand(hole0, community);
  auto hand1 = evaluate_hand(hole1, community);

  EXPECT_TRUE(hand0 > hand1);
  EXPECT_FALSE(hand0 == hand1);
}

TEST(HandEvaluationTest, IdenticalHands) {
  std::array<Card, 2> hole0 = {make_card_helper(0, 2), make_card_helper(1, 3)};
  std::array<Card, 2> hole1 = {make_card_helper(2, 4), make_card_helper(3, 5)};

  // Board has pair of Aces - both players play the board
  std::array<Card, 5> community = {
      make_card_helper(0, 1), make_card_helper(1, 1), make_card_helper(2, 13),
      make_card_helper(3, 12), make_card_helper(0, 11)};

  auto hand0 = evaluate_hand(hole0, community);
  auto hand1 = evaluate_hand(hole1, community);

  EXPECT_TRUE(hand0 == hand1);
}

// Test side pot calculation
TEST(SidePotTest, SimpleSidePot) {
  BoardState<3> state;
  state.n_players = 3;

  // Player 0: all-in for 100
  state.players[0].bet = 100;
  state.players[0].state = PlayerState::State::ALL_IN;

  // Players 1 and 2: bet 200
  state.players[1].bet = 200;
  state.players[1].state = PlayerState::State::ACTIVE;
  state.players[2].bet = 200;
  state.players[2].state = PlayerState::State::ACTIVE;

  auto pots = calculate_side_pots(state);

  // Should have 2 pots:
  // Main pot: 100 * 3 = 300 (all players eligible)
  // Side pot: 100 * 2 = 200 (only players 1 and 2 eligible)
  ASSERT_EQ(pots.size(), 2);
  EXPECT_EQ(pots[0].amount, 300);
  EXPECT_EQ(pots[0].eligible_players.size(), 3);
  EXPECT_EQ(pots[1].amount, 200);
  EXPECT_EQ(pots[1].eligible_players.size(), 2);
}

TEST(SidePotTest, NoSidePot) {
  BoardState<2> state;
  state.n_players = 2;

  state.players[0].bet = 100;
  state.players[0].state = PlayerState::State::ACTIVE;
  state.players[1].bet = 100;
  state.players[1].state = PlayerState::State::ACTIVE;

  auto pots = calculate_side_pots(state);

  // Should have 1 pot
  ASSERT_EQ(pots.size(), 1);
  EXPECT_EQ(pots[0].amount, 200);
  EXPECT_EQ(pots[0].eligible_players.size(), 2);
}

TEST(SidePotTest, FoldedPlayerExcluded) {
  BoardState<3> state;
  state.n_players = 3;

  state.players[0].bet = 100;
  state.players[0].state = PlayerState::State::FOLDED;
  state.players[1].bet = 100;
  state.players[1].state = PlayerState::State::ACTIVE;
  state.players[2].bet = 100;
  state.players[2].state = PlayerState::State::ACTIVE;

  auto pots = calculate_side_pots(state);

  // Folded player shouldn't be in any pot
  ASSERT_EQ(pots.size(), 1);
  EXPECT_EQ(pots[0].amount, 200); // Only players 1 and 2
  EXPECT_EQ(pots[0].eligible_players.size(), 2);
}

// Test full showdown resolution
TEST(ShowdownResolutionTest, SimpleWinner) {
  BoardState<2> state;
  state.n_players = 2;

  // Setup players
  state.players[0].cards = {make_card_helper(0, 1),
                            make_card_helper(1, 1)}; // Aces
  state.players[0].stack = 980;                      // After betting 20
  state.players[0].bet = 20;
  state.players[0].state = PlayerState::State::ACTIVE;

  state.players[1].cards = {make_card_helper(0, 13),
                            make_card_helper(1, 13)}; // Kings
  state.players[1].stack = 980;
  state.players[1].bet = 20;
  state.players[1].state = PlayerState::State::ACTIVE;

  // Community cards
  state.openCards = {make_card_helper(2, 2), make_card_helper(3, 4),
                     make_card_helper(0, 6), make_card_helper(1, 7),
                     make_card_helper(2, 8)};

  state.pot = 40;

  // Run showdown
  finalise_showdown_impl(state);

  // Player 0 should win with Aces
  EXPECT_EQ(state.player_rewards[0], 40);
  EXPECT_EQ(state.player_rewards[1], 0);

  // Stacks should be updated
  EXPECT_EQ(state.players[0].stack, 1020); // 980 + 40
  EXPECT_EQ(state.players[1].stack, 980);

  // Bets should be reset
  EXPECT_EQ(state.players[0].bet, 0);
  EXPECT_EQ(state.players[1].bet, 0);
  EXPECT_EQ(state.pot, 0);
}

TEST(ShowdownResolutionTest, SplitPot) {
  BoardState<2> state;
  state.n_players = 2;

  // Both players have low cards - will play the board
  state.players[0].cards = {make_card_helper(0, 2), make_card_helper(1, 3)};
  state.players[0].stack = 980;
  state.players[0].bet = 20;
  state.players[0].state = PlayerState::State::ACTIVE;

  state.players[1].cards = {make_card_helper(2, 4), make_card_helper(3, 5)};
  state.players[1].stack = 980;
  state.players[1].bet = 20;
  state.players[1].state = PlayerState::State::ACTIVE;

  // Board has pair of Aces
  state.openCards = {make_card_helper(0, 1), make_card_helper(1, 1),
                     make_card_helper(2, 13), make_card_helper(3, 12),
                     make_card_helper(0, 11)};

  state.pot = 40;

  finalise_showdown_impl(state);

  // Pot should be split
  EXPECT_EQ(state.player_rewards[0], 20);
  EXPECT_EQ(state.player_rewards[1], 20);

  EXPECT_EQ(state.players[0].stack, 1000);
  EXPECT_EQ(state.players[1].stack, 1000);
}

TEST(ShowdownResolutionTest, SidePotWinner) {
  BoardState<3> state;
  state.n_players = 3;

  // Player 0: all-in with Aces (best hand - wins main pot only)
  state.players[0].cards = {make_card_helper(0, 1), make_card_helper(1, 1)};
  state.players[0].stack = 0;
  state.players[0].bet = 100;
  state.players[0].state = PlayerState::State::ALL_IN;

  // Player 1: Queens (worst hand)
  state.players[1].cards = {make_card_helper(0, 12), make_card_helper(1, 12)};
  state.players[1].stack = 800;
  state.players[1].bet = 200;
  state.players[1].state = PlayerState::State::ACTIVE;

  // Player 2: Kings (middle hand - wins side pot)
  state.players[2].cards = {make_card_helper(0, 13), make_card_helper(1, 13)};
  state.players[2].stack = 800;
  state.players[2].bet = 200;
  state.players[2].state = PlayerState::State::ACTIVE;

  state.openCards = {make_card_helper(2, 2), make_card_helper(3, 4),
                     make_card_helper(0, 6), make_card_helper(1, 7),
                     make_card_helper(2, 8)};

  state.pot = 500;

  finalise_showdown_impl(state);

  // Main pot (300): Player 0 wins with Aces (best among all 3)
  // Side pot (200): Player 2 wins with Kings (best among players 1 and 2)
  EXPECT_EQ(state.player_rewards[0], 300);
  EXPECT_EQ(state.player_rewards[1], 0);
  EXPECT_EQ(state.player_rewards[2], 200);
}
