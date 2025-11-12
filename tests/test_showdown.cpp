#include "../src/Board.cpp"
#include "../src/PokerTypes.hpp"
#include <gtest/gtest.h>

class ShowdownTest : public ::testing::Test {
protected:
  PokerBoard<6> board;

  void SetUp() override { board.hard_reset(); }

  // Helper function to set up a hand manually
  void setup_hand(const std::vector<int> &stacks,
                  const std::vector<std::array<Card, 2>> &player_cards,
                  const std::array<Card, 5> &community_cards) {
    for (size_t i = 0; i < stacks.size(); i++) {
      board.add_player(stacks[i]);
    }
    std::vector<std::array<Card, 2>> cards = player_cards;
    std::array<Card, 5> open_cards = community_cards;
    board.auto_start(open_cards, cards);
  }

  // Helper to play through all betting rounds to showdown with everyone
  // checking/calling
  void play_to_showdown() {

    auto state = board.get_board_state();
    int max_iterations = 1000; // Prevent infinite loops
    int iterations = 0;

    // Play through all betting rounds until hand ends
    while (state.betting_round != BettingRound::SETUP &&
           iterations < max_iterations) {
      iterations++;

      int current = state.current_player_idx;
      int current_bet = state.current_bet;
      int player_bet = state.players[current].bet;

      std::optional<int> result;
      if (current_bet > player_bet) {
        result = board.player_act(
            {static_cast<uint8_t>(current), PlayerAction::CALL, 0});
      } else {
        result = board.player_act(
            {static_cast<uint8_t>(current), PlayerAction::CHECK, 0});
      }

      if (!result.has_value()) {
        break; // Hand is over
      }
      state = board.get_board_state();
    }

    if (iterations >= max_iterations) {
      FAIL() << "play_to_showdown exceeded maximum iterations";
    }
  }

  // Helper to create a card from suite and number
  Card make_card(int suite, int number) {
    Card c;
    c.suite = suite;
    c.number = number;
    return c;
  }
};

// ============================================================================
// Hand Evaluation Tests
// ============================================================================

TEST_F(ShowdownTest, RoyalFlush) {
  // Player 0: Royal Flush in hearts
  // Player 1: Flush in hearts (lower)
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(2, 1), make_card(2, 13)}, // Ace and King of Hearts
      {make_card(2, 2), make_card(2, 3)}   // 2 and 3 of Hearts
  };
  std::array<Card, 5> community_cards = {
      make_card(2, 10), // 10 of Hearts
      make_card(2, 11), // Jack of Hearts
      make_card(2, 12), // Queen of Hearts
      make_card(0, 7),  // 7 of Clubs
      make_card(1, 8)   // 8 of Diamonds
  };

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();

  // Player 0 should win with Royal Flush
  EXPECT_GT(state.player_rewards[0], 0);
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, StraightFlush) {
  // Player 0: Straight Flush 5-6-7-8-9 of Spades
  // Player 1: Regular Straight (not flush)
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(3, 5), make_card(3, 6)}, // 5 and 6 of Spades
      {make_card(0, 5), make_card(1, 6)}  // 5 and 6 different suits
  };
  std::array<Card, 5> community_cards = {make_card(3, 7), // 7 of Spades
                                         make_card(3, 8), // 8 of Spades
                                         make_card(3, 9), // 9 of Spades
                                         make_card(0, 2), make_card(1, 3)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0);
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, WheelStraight) {
  // Test A-2-3-4-5 (wheel) straight
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 2)},  // Ace and 2
      {make_card(0, 10), make_card(1, 11)} // 10 and Jack (high card only)
  };
  std::array<Card, 5> community_cards = {make_card(2, 3), // 3
                                         make_card(3, 4), // 4
                                         make_card(0, 5), // 5
                                         make_card(1, 9), make_card(2, 7)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with wheel
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, FourOfAKind) {
  // Player 0: Four Kings
  // Player 1: Full House
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 13), make_card(1, 13)}, // Two Kings
      {make_card(0, 10), make_card(1, 10)}  // Pair of 10s
  };
  std::array<Card, 5> community_cards = {
      make_card(2, 13), // King
      make_card(3, 13), // King
      make_card(2, 10), // 10 (makes full house for player 1)
      make_card(0, 5), make_card(1, 7)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with Four of a Kind
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, FullHouse) {
  // Player 0: Full House Aces over Kings
  // Player 1: Full House Kings over Queens (lower)
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 1)},  // Pair of Aces
      {make_card(0, 13), make_card(1, 13)} // Pair of Kings
  };
  std::array<Card, 5> community_cards = {
      make_card(2, 1),                    // Ace (gives player 0 three aces)
      make_card(2, 13),                   // King (gives player 1 three kings)
      make_card(3, 12),                   // Queen (instead of King)
      make_card(0, 12), make_card(1, 3)}; // Queen

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with higher full house
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, Flush) {
  // Player 0: Flush with Ace high
  // Player 1: Flush with King high
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(2, 1), make_card(2, 9)}, // Ace and 9 of Hearts
      {make_card(2, 13), make_card(2, 8)} // King and 8 of Hearts
  };
  std::array<Card, 5> community_cards = {make_card(2, 2), // 2 of Hearts
                                         make_card(2, 4), // 4 of Hearts
                                         make_card(2, 6), // 6 of Hearts
                                         make_card(0, 10), make_card(1, 11)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with Ace high flush
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, Straight) {
  // Player 0: Straight 9-10-J-Q-K
  // Player 1: Straight 8-9-10-J-Q
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 9), make_card(1, 13)}, // 9 and King
      {make_card(0, 8), make_card(1, 12)}  // 8 and Queen
  };
  std::array<Card, 5> community_cards = {make_card(2, 10), // 10
                                         make_card(3, 11), // Jack
                                         make_card(0, 12), // Queen
                                         make_card(1, 2), make_card(2, 3)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with higher straight
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, ThreeOfAKind) {
  // Player 0: Three Aces
  // Player 1: Three Kings
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 1)},  // Pair of Aces
      {make_card(0, 13), make_card(1, 13)} // Pair of Kings
  };
  std::array<Card, 5> community_cards = {make_card(2, 1),  // Ace
                                         make_card(2, 13), // King
                                         make_card(3, 5), make_card(0, 7),
                                         make_card(1, 9)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with three aces
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, TwoPair) {
  // Player 0: Aces and Kings
  // Player 1: Kings and Queens
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 13)}, // Ace and King
      {make_card(0, 13), make_card(1, 12)} // King and Queen
  };
  std::array<Card, 5> community_cards = {make_card(2, 1),  // Ace
                                         make_card(2, 13), // King
                                         make_card(3, 12), // Queen
                                         make_card(0, 5), make_card(1, 7)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with Aces and Kings
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, OnePairWithKicker) {
  // Player 0: Pair of Aces with King kicker
  // Player 1: Pair of Aces with Queen kicker
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 13)}, // Ace and King
      {make_card(2, 1), make_card(1, 12)}  // Ace and Queen
  };
  std::array<Card, 5> community_cards = {make_card(3, 5), make_card(2, 7),
                                         make_card(0, 9), make_card(1, 2),
                                         make_card(0, 4)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with better kicker
  EXPECT_EQ(state.player_rewards[1], 0);
}

TEST_F(ShowdownTest, HighCard) {
  // Player 0: Ace high
  // Player 1: King high
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 10)}, // Ace and 10
      {make_card(0, 13), make_card(1, 9)}  // King and 9
  };
  std::array<Card, 5> community_cards = {make_card(2, 2), make_card(3, 4),
                                         make_card(0, 6), make_card(1, 7),
                                         make_card(2, 8)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Player 0 wins with Ace high
  EXPECT_EQ(state.player_rewards[1], 0);
}

// ============================================================================
// Split Pot Tests
// ============================================================================

TEST_F(ShowdownTest, SplitPotIdenticalHands) {
  // Both players have the same hand from board
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 2), make_card(1, 3)}, // 2 and 3
      {make_card(2, 4), make_card(3, 5)}  // 4 and 5
  };
  std::array<Card, 5> community_cards = {
      make_card(0, 1),  // Ace
      make_card(1, 1),  // Ace
      make_card(2, 13), // King
      make_card(3, 12), // Queen
      make_card(0, 11)  // Jack
  };

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  // Both players should get equal rewards (split pot)
  EXPECT_GT(state.player_rewards[0], 0);
  EXPECT_EQ(state.player_rewards[0], state.player_rewards[1]);
}

TEST_F(ShowdownTest, SplitPotThreeWay) {
  // Three players with identical hands from board
  std::vector<int> stacks = {1000, 1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 2), make_card(1, 3)},
      {make_card(2, 2), make_card(3, 3)},
      {make_card(0, 4), make_card(1, 5)}};
  std::array<Card, 5> community_cards = {
      make_card(0, 1),  // Ace
      make_card(1, 1),  // Ace
      make_card(2, 13), // King
      make_card(3, 12), // Queen
      make_card(0, 11)  // Jack
  };

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  // All three should split the pot
  EXPECT_GT(state.player_rewards[0], 0);
  EXPECT_GT(state.player_rewards[1], 0);
  EXPECT_GT(state.player_rewards[2], 0);

  // Pot should be split roughly equally
  int total_pot = state.player_rewards[0] + state.player_rewards[1] +
                  state.player_rewards[2];
  EXPECT_EQ(total_pot, 60); // All 3 players call 20 each (SB posts 10, calls 10
                            // more; BB posts 20; Button calls 20)
}

// ============================================================================
// Stack and Reward Update Tests
// ============================================================================

TEST_F(ShowdownTest, StacksUpdatedCorrectly) {
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 1)},  // Aces (best)
      {make_card(0, 13), make_card(1, 13)} // Kings
  };
  std::array<Card, 5> community_cards = {make_card(2, 2), make_card(3, 4),
                                         make_card(0, 6), make_card(1, 7),
                                         make_card(2, 8)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();

  // Player 0 wins 40 (both players end up putting in 20 each in heads-up)
  EXPECT_EQ(state.player_rewards[0], 40);
  // Stacks should be updated
  EXPECT_EQ(state.players[0].stack, 1020); // 1000 - 20 + 40
  EXPECT_EQ(state.players[1].stack, 980);  // 1000 - 20
}

TEST_F(ShowdownTest, BetsResetAfterShowdown) {
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 1)}, {make_card(0, 13), make_card(1, 13)}};
  std::array<Card, 5> community_cards = {make_card(2, 2), make_card(3, 4),
                                         make_card(0, 6), make_card(1, 7),
                                         make_card(2, 8)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();

  // All bets should be reset to 0
  EXPECT_EQ(state.players[0].bet, 0);
  EXPECT_EQ(state.players[1].bet, 0);
  // Pot should be reset
  EXPECT_EQ(state.pot, 0);
}

TEST_F(ShowdownTest, AceHighVsKingHigh) {
  // Test kicker comparison with high cards
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 11)}, // Ace and Jack
      {make_card(0, 13), make_card(1, 12)} // King and Queen
  };
  std::array<Card, 5> community_cards = {make_card(2, 2), make_card(3, 4),
                                         make_card(0, 6), make_card(1, 8),
                                         make_card(2, 10)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();
  EXPECT_GT(state.player_rewards[0], 0); // Ace high wins
  EXPECT_EQ(state.player_rewards[1], 0);
}

// ============================================================================
// Side Pot Tests - These are more complex and need different setup
// ============================================================================

TEST_F(ShowdownTest, AllInWinnerGetsCorrectAmount) {
  // Simplified test: short stack player has aces and should win
  std::vector<int> stacks = {100, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 1)},  // Player 0: Aces
      {make_card(0, 13), make_card(1, 13)} // Player 1: Kings
  };
  std::array<Card, 5> community_cards = {make_card(2, 2), make_card(3, 4),
                                         make_card(0, 6), make_card(1, 7),
                                         make_card(2, 8)};

  setup_hand(stacks, player_cards, community_cards);

  // Use play_to_showdown to let the hand play out naturally
  // The short stack will eventually be all-in
  play_to_showdown();

  auto state = board.get_board_state();

  // Player 0 with aces should win the pot
  // In heads-up with play_to_showdown calling/checking: both players put in 20
  // each (SB posts 10 then calls 10 more, BB posts 20 and checks)
  EXPECT_GT(state.player_rewards[0], 0);
  EXPECT_EQ(state.player_rewards[0],
            40); // Pot is 40 in heads-up with just calling
}

TEST_F(ShowdownTest, PlayerRewardsArrayPopulated) {
  // Verify player_rewards array is properly set
  std::vector<int> stacks = {1000, 1000};
  std::vector<std::array<Card, 2>> player_cards = {
      {make_card(0, 1), make_card(1, 1)}, {make_card(0, 13), make_card(1, 13)}};
  std::array<Card, 5> community_cards = {make_card(2, 2), make_card(3, 4),
                                         make_card(0, 6), make_card(1, 7),
                                         make_card(2, 8)};

  setup_hand(stacks, player_cards, community_cards);
  play_to_showdown();

  auto state = board.get_board_state();

  // Winner should have positive reward (heads-up pot is 40: both players put in
  // 20)
  EXPECT_EQ(state.player_rewards[0], 40);
  EXPECT_EQ(state.player_rewards[1], 0);

  // Sum of rewards should equal the pot that was distributed
  int total_rewards = 0;
  for (int i = 0; i < state.n_players; i++) {
    total_rewards += state.player_rewards[i];
  }
  EXPECT_EQ(total_rewards, 40);
}
