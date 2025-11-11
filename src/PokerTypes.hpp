#pragma once
#include <array>
#include <cstdint>

struct Card {
  uint8_t suite : 2;
  uint8_t number : 6;

  Card() = default;

  // Implicit conversion from DeckCard
  Card(uint8_t deck_card) {
    suite = deck_card >> 6;
    number = deck_card & 0b00111111;
  }

  bool operator==(const Card &other) const {
    return suite == other.suite && number == other.number;
  }
};

enum DeckCard : uint8_t {
  // Clubs (suite = 0)
  ACE_CLUBS = (0 << 6) | 1,
  TWO_CLUBS = (0 << 6) | 2,
  THREE_CLUBS = (0 << 6) | 3,
  FOUR_CLUBS = (0 << 6) | 4,
  FIVE_CLUBS = (0 << 6) | 5,
  SIX_CLUBS = (0 << 6) | 6,
  SEVEN_CLUBS = (0 << 6) | 7,
  EIGHT_CLUBS = (0 << 6) | 8,
  NINE_CLUBS = (0 << 6) | 9,
  TEN_CLUBS = (0 << 6) | 10,
  JACK_CLUBS = (0 << 6) | 11,
  QUEEN_CLUBS = (0 << 6) | 12,
  KING_CLUBS = (0 << 6) | 13,

  // Diamonds (suite = 1)
  ACE_DIAMONDS = (1 << 6) | 1,
  TWO_DIAMONDS = (1 << 6) | 2,
  THREE_DIAMONDS = (1 << 6) | 3,
  FOUR_DIAMONDS = (1 << 6) | 4,
  FIVE_DIAMONDS = (1 << 6) | 5,
  SIX_DIAMONDS = (1 << 6) | 6,
  SEVEN_DIAMONDS = (1 << 6) | 7,
  EIGHT_DIAMONDS = (1 << 6) | 8,
  NINE_DIAMONDS = (1 << 6) | 9,
  TEN_DIAMONDS = (1 << 6) | 10,
  JACK_DIAMONDS = (1 << 6) | 11,
  QUEEN_DIAMONDS = (1 << 6) | 12,
  KING_DIAMONDS = (1 << 6) | 13,

  // Hearts (suite = 2)
  ACE_HEARTS = (2 << 6) | 1,
  TWO_HEARTS = (2 << 6) | 2,
  THREE_HEARTS = (2 << 6) | 3,
  FOUR_HEARTS = (2 << 6) | 4,
  FIVE_HEARTS = (2 << 6) | 5,
  SIX_HEARTS = (2 << 6) | 6,
  SEVEN_HEARTS = (2 << 6) | 7,
  EIGHT_HEARTS = (2 << 6) | 8,
  NINE_HEARTS = (2 << 6) | 9,
  TEN_HEARTS = (2 << 6) | 10,
  JACK_HEARTS = (2 << 6) | 11,
  QUEEN_HEARTS = (2 << 6) | 12,
  KING_HEARTS = (2 << 6) | 13,

  // Spades (suite = 3)
  ACE_SPADES = (3 << 6) | 1,
  TWO_SPADES = (3 << 6) | 2,
  THREE_SPADES = (3 << 6) | 3,
  FOUR_SPADES = (3 << 6) | 4,
  FIVE_SPADES = (3 << 6) | 5,
  SIX_SPADES = (3 << 6) | 6,
  SEVEN_SPADES = (3 << 6) | 7,
  EIGHT_SPADES = (3 << 6) | 8,
  NINE_SPADES = (3 << 6) | 9,
  TEN_SPADES = (3 << 6) | 10,
  JACK_SPADES = (3 << 6) | 11,
  QUEEN_SPADES = (3 << 6) | 12,
  KING_SPADES = (3 << 6) | 13
};

enum class PlayerAction {
  FOLD,
  CHECK,
  BET,
  CALL,
  RAISE,
  ALL_IN,
};

struct Action {
  uint8_t player_id;
  PlayerAction action;
  int amount;
};
enum class BettingRound : uint8_t {
  SETUP,
  PREFLOP,
  FLOP,
  TURN,
  RIVER,
  SHOWDOWN
};

struct PlayerState {
  enum State { ACTIVE, INACTIVE, FOLDED, ALL_IN };
  std::array<Card, 2> cards;
  int stack;
  int bet;
  State state;
};

template <int N> struct BoardState {
  std::array<Card, 5> openCards;
  std::array<PlayerState, N> players;
  std::array<int, N> player_rewards;
  BettingRound betting_round;
  int n_players = 0;
  int pot = 0;
  int current_bet = 0;
  int dealer_button_idx = 0;  // index in players vector
  int current_player_idx = 0; // index in players vector
  int last_aggressor_idx =
      -1; // index of player who last bet/raised (-1 if none)
  int big_amount = 20;
  int small_amount = 10;
};
