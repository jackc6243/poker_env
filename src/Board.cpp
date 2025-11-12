#include "PokerTypes.hpp"
#include "resolve_showdown.cpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>

template <int N> void finalise_showdown_impl(BoardState<N> &state);

template <int N> class PokerBoard {
public:
  PokerBoard(int big_blind = 20, int small_blind = 10)
      : rng_(std::random_device{}()), small_amount_(small_blind),
        big_amount_(big_blind) {
    state = get_default_state();

    // Initialize deck
    for (int suite = 0; suite < 4; suite++) {
      for (int number = 1; number <= 13; number++) {
        Card card;
        card.suite = suite;
        card.number = number;
        deck_.push_back(card);
      }
    }
  }

  // hard reset to base default state
  void hard_reset() { state = get_default_state(); }

  // returns the index the player adds
  int add_player(int stack = 0) {
    if (state->betting_round != BettingRound::SETUP) {
      throw std::runtime_error("You can only add players during setup");
    }
    if (state->n_players >= N) {
      throw std::runtime_error("too many players, cannot join anymore");
    }
    PlayerState player;
    player.stack = stack;
    player.state = PlayerState::State::ACTIVE;
    state->players[state->n_players] = player;
    state->n_players++;
    return state->n_players - 1;
  }

  // topup player stack and returns the new amount;
  int player_stack_topup(int idx, int amount) {
    state->players[idx].stack += amount;
    return state->players[idx].stack;
  }

  PlayerState remove_player(int idx) {
    if (state->betting_round != BettingRound::SETUP) {
      throw std::runtime_error("You can only remove players during setup");
    }
    check_valid_index(idx);
    auto toRemove = state->players[idx];
    for (int i = idx; i < state->n_players - 1; i++) {
      state->players[i] = state->players[i + 1];
    }
    state->n_players--;
    return toRemove;
  }

  void activate_player(int idx) {
    check_valid_index(idx);
    if (state->players[idx].state == PlayerState::State::INACTIVE) {
      state->players[idx].state = PlayerState::State::ACTIVE;
    }
  }

  void deactivate_player(int idx) {
    check_valid_index(idx);
    if (state->players[idx].state == PlayerState::State::ACTIVE) {
      state->players[idx].state = PlayerState::State::INACTIVE;
    }
  }

  void auto_start() {
    remove_inactive_players();
    reset_hand_random();
    start_hand();
    post_blinds();
  }

  void auto_start(std::array<Card, 5> &open_cards,
                  std::vector<std::array<Card, 2>> &player_cards) {
    remove_inactive_players();
    reset_hand_manual(open_cards, player_cards);
    start_hand();
    post_blinds();
  }

  std::optional<int> player_act(Action action) {
    const auto idx = action.player_id;
    check_correct_player(idx);

    if (state->players[state->current_player_idx].state ==
        PlayerState::INACTIVE) {
      throw std::runtime_error("Inactive player can't make any actions");
    }

    PlayerState &player = get_player(idx);

    switch (action.action) {
    case PlayerAction::FOLD:
      player_fold(player);
      break;

    case PlayerAction::CHECK:
      player_check(player);
      break;

    case PlayerAction::BET:
      player_bet(player, action.amount);
      break;

    case PlayerAction::CALL:
      player_call(player);
      break;

    case PlayerAction::RAISE:
      player_raise(player, action.amount);
      break;

    case PlayerAction::ALL_IN:
      player_all_in(player);
      break;
    }

    return advance_player_idx();
  }

  // Get player state by index
  PlayerState &get_player(int idx) const {
    check_valid_index(idx);
    return state->players[idx];
  }

  bool hand_in_progress() {
    return state->betting_round != BettingRound::SETUP;
  };

  const BoardState<N> &get_board_state() const { return *state; }

private:
  void remove_inactive_players() {
    for (int i = state->n_players - 1; i >= 0; i--) {
      if (state->players[i].state == PlayerState::State::INACTIVE) {
        // Shift all players after this one down
        for (int j = i; j < state->n_players - 1; j++) {
          state->players[j] = state->players[j + 1];
        }
        state->n_players--;
      }
    }
  }

  void start_hand() {
    if (state->betting_round != BettingRound::SETUP) {
      throw std::runtime_error("Can only start from SETUP betting round");
    }
    state->betting_round = BettingRound::PREFLOP;
    state->dealer_button_idx =
        (state->dealer_button_idx + 1) % state->n_players;
  }

  void reset_hand_random() {
    if (state->n_players == 0) {
      throw std::runtime_error("Cannot deal hand with no players");
    }

    std::shuffle(deck_.begin(), deck_.end(), rng_);

    // Deal 2 cards to each player
    int deck_idx = 0;
    for (int i = 0; i < state->n_players; i++) {
      auto &player = state->players[i];
      player.cards[0] = deck_[deck_idx++];
      player.cards[1] = deck_[deck_idx++];
      player.bet = 0;
      player.state = PlayerState::State::ACTIVE;
    }

    for (int i = 0; i < 5; i++) {
      state->openCards[i] = deck_[deck_idx++];
    }

    state->pot = 0;
    state->current_bet = 0;
  }

  // manualy reset hand, must either call this or reset_hand_random before
  // starting
  void reset_hand_manual(std::array<Card, 5> &open_cards,
                         std::vector<std::array<Card, 2>> &player_cards) {
    if (state->n_players == 0) {
      throw std::runtime_error("Cannot reset hand with no players");
    }
    if (state->n_players != player_cards.size()) {
      throw std::runtime_error(
          "Number of players are different from the cards given");
    }
    state->openCards = open_cards;
    for (int i = 0; i < state->n_players; i++) {
      auto &player = state->players[i];
      player.cards = player_cards[i];
      player.bet = 0;
      player.state = PlayerState::State::ACTIVE;
    }

    state->pot = 0;
    state->current_bet = 0;
  }

  void post_blinds() {
    if (state->n_players < 2)
      throw std::runtime_error("We need at least 2 players to post blinds");

    // Calculate blind positions
    // In heads-up (2 players), dealer is small blind
    // In 3+ players, player after dealer is small blind
    int small_blind_idx, big_blind_idx;

    if (state->n_players == 2) {
      // Heads-up: dealer is small blind
      small_blind_idx = state->dealer_button_idx;
      big_blind_idx = (state->dealer_button_idx + 1) % state->n_players;
    } else {
      // 3+ players: normal positions
      small_blind_idx = (state->dealer_button_idx + 1) % state->n_players;
      big_blind_idx = (state->dealer_button_idx + 2) % state->n_players;
    }

    // here we are keepting the inavriant that both small and big blind
    // players must be active
    int sb_amount = 0;
    if (state->players[small_blind_idx].stack < state->small_amount) {
      throw std::runtime_error("Small blind player does not have enough chips");
    }
    // Post small blind
    sb_amount =
        std::min(state->small_amount, state->players[small_blind_idx].stack);
    state->players[small_blind_idx].stack -= sb_amount;
    state->players[small_blind_idx].bet = sb_amount;
    if (state->players[small_blind_idx].stack == 0) {
      state->players[small_blind_idx].state = PlayerState::State::ALL_IN;
    }

    int bb_amount = 0;
    if (state->players[big_blind_idx].stack < state->big_amount) {
      throw std::runtime_error("Big blind player does not have enough chips");
    }
    // Post big blind
    bb_amount =
        std::min(state->big_amount, state->players[big_blind_idx].stack);
    state->players[big_blind_idx].stack -= bb_amount;
    state->players[big_blind_idx].bet = bb_amount;
    if (state->players[big_blind_idx].stack == 0) {
      state->players[big_blind_idx].state = PlayerState::State::ALL_IN;
    }

    state->pot = sb_amount + bb_amount;
    state->current_bet = std::max(sb_amount, bb_amount);
    state->last_aggressor_idx = big_blind_idx;

    // Set first to act: in heads-up, small blind (dealer) acts first
    // In 3+ players, first player after big blind acts first
    if (state->n_players == 2) {
      state->current_player_idx =
          small_blind_idx; // Dealer/small blind acts first in heads-up
    } else {
      state->current_player_idx = (big_blind_idx + 1) % state->n_players;
    }
  }

  void player_fold(PlayerState &player) {
    player.state = PlayerState::State::FOLDED;
    state->last_aggressor_idx = next_active_player(state->current_player_idx);
  }

  void player_check(PlayerState &player) {
    // Can only check if current bet equals player's bet
    if (player.bet != state->current_bet) {
      throw std::runtime_error("Cannot check - must call or raise");
    }
  }

  void player_all_in(PlayerState &player) {
    // Add entire stack to bet
    int all_in_amount = player.stack;
    place_bet(player, all_in_amount);
    player.state = PlayerState::State::ALL_IN;
    state->last_aggressor_idx = next_active_player(state->current_player_idx);
  }

  void player_bet(PlayerState &player, int amount) {
    // Can only bet if no one has bet yet
    if (state->current_bet > player.bet) {
      throw std::runtime_error("Cannot bet - must call or raise");
    }

    // Check if player has enough chips
    if (player.stack < amount) {
      throw std::runtime_error("Not enough chips");
    }

    place_bet(player, amount);
    state->last_aggressor_idx = state->current_player_idx;
  }

  void player_call(PlayerState &player) {

    // Calculate amount needed to call
    int call_amount = state->current_bet - player.bet;

    // Check if player has enough chips
    if (player.stack < call_amount) {
      throw std::runtime_error("Not enough chips");
    }

    place_bet(player, call_amount);
  }

  void player_raise(PlayerState &player, int amount) {

    // Raise must be higher than current bet
    if (amount <= state->current_bet) {
      throw std::runtime_error("Raise amount must be higher than current bet");
    }

    int additional_amount = amount - player.bet;
    if (player.stack < additional_amount) {
      throw std::runtime_error("Not enough chips");
    }

    place_bet(player, additional_amount);
    state->last_aggressor_idx = state->current_player_idx;
  }

  void finalise_showdown() { finalise_showdown_impl(*state); }

  BettingRound advance_betting_round() {
    // Advance to next betting round
    if (state->betting_round == BettingRound::RIVER ||
        state->betting_round == BettingRound::SHOWDOWN) {
      // After river, go to showdown and finalize
      finalise_showdown();
      state->betting_round = BettingRound::SETUP;
    } else {
      state->betting_round =
          static_cast<BettingRound>(static_cast<int>(state->betting_round) + 1);
    }

    state->current_player_idx = next_active_player(state->current_player_idx);
    state->last_aggressor_idx = state->current_player_idx;
    return state->betting_round;
  };

  int next_active_player(int start_idx) {
    int next_idx = start_idx;
    do {
      next_idx = (next_idx + 1) % state->n_players;

      if (state->players[next_idx].state == PlayerState::State::INACTIVE) {
        state->players[next_idx].state = PlayerState::State::FOLDED;
      }
      if (next_idx == start_idx) {
        break;
      }
    } while (state->players[next_idx].state != PlayerState::State::ACTIVE);
    return next_idx;
  }

  int n_active_players() {
    int n = 0;
    for (int i = 0; i < state->n_players; i++) {
      auto &player = state->players[i];
      if (player.state == PlayerState::ACTIVE ||
          player.state == PlayerState::ALL_IN) {
        n++;
      }
    }
    return n;
  }

  // returns std::nullopt if hand is over, or player_idx if otherwise
  std::optional<int> advance_player_idx() {
    int cur_player_idx = state->current_player_idx;
    int next_active_idx = next_active_player(cur_player_idx);

    // If only <=1 active player, finish the game immediately
    if (n_active_players() <= 1) {
      finalise_showdown();
      state->betting_round = BettingRound::SETUP;
      return std::nullopt;
    }

    // Check if we've completed the betting round
    if (state->last_aggressor_idx == next_active_idx) {
      return (advance_betting_round() == BettingRound::SETUP)
                 ? std::nullopt
                 : std::optional<int>(state->current_player_idx);
    }

    return state->current_player_idx = next_active_idx;
  }

  inline void place_bet(PlayerState &player, int amount) {
    player.stack -= amount;
    player.bet += amount;
    state->pot += amount;
    state->current_bet = std::max(state->current_bet, player.bet);
  }

  inline void check_valid_index(int idx) const {
    if (idx < 0 || idx >= state->n_players) {
      throw std::runtime_error("Player index out of bounds");
    }
  }

  inline void check_correct_player(int idx) {
    if (idx != state->current_player_idx) {
      throw std::runtime_error("Not this player's turn");
    }
  }

  std::unique_ptr<BoardState<N>> get_default_state() {
    auto state = std::make_unique<BoardState<N>>();
    state->betting_round = BettingRound::SETUP;
    state->big_amount = big_amount_;
    state->small_amount = small_amount_;
    state->dealer_button_idx = -1;
    return state;
  };

  std::unique_ptr<BoardState<N>> state;
  std::vector<Card> deck_;
  std::mt19937 rng_;
  int small_amount_;
  int big_amount_;
};
