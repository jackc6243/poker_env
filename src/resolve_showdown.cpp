#include "PokerTypes.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

// Hand ranking enum (higher value = better hand)
enum class HandRank : uint8_t {
  HIGH_CARD = 0,
  ONE_PAIR = 1,
  TWO_PAIR = 2,
  THREE_OF_A_KIND = 3,
  STRAIGHT = 4,
  FLUSH = 5,
  FULL_HOUSE = 6,
  FOUR_OF_A_KIND = 7,
  STRAIGHT_FLUSH = 8,
  ROYAL_FLUSH = 9
};

// Evaluated hand with ranking and kickers for comparison
struct HandValue {
  HandRank rank;
  std::array<uint8_t, 5> kickers; // Tiebreakers in order of importance

  bool operator>(const HandValue &other) const {
    if (rank != other.rank)
      return rank > other.rank;
    return kickers > other.kickers;
  }

  bool operator==(const HandValue &other) const {
    return rank == other.rank && kickers == other.kickers;
  }
};

// Evaluate the best 5-card poker hand from 7 cards (2 hole + 5 community)
HandValue evaluate_hand(const std::array<Card, 2> &hole_cards,
                        const std::array<Card, 5> &community_cards) {
  std::array<Card, 7> all_cards;
  all_cards[0] = hole_cards[0];
  all_cards[1] = hole_cards[1];
  for (int i = 0; i < 5; i++) {
    all_cards[i + 2] = community_cards[i];
  }

  // Count cards by rank (treating Ace as 14 for high straight/pair evaluation)
  std::array<int, 15> rank_counts = {};
  std::array<int, 4> suit_counts = {};

  for (const auto &card : all_cards) {
    rank_counts[card.number]++;
    suit_counts[card.suite]++;
  }

  // Find flush suit (if any)
  int flush_suit = -1;
  for (int i = 0; i < 4; i++) {
    if (suit_counts[i] >= 5) {
      flush_suit = i;
      break;
    }
  }

  // Collect cards of flush suit
  std::vector<uint8_t> flush_ranks;
  if (flush_suit != -1) {
    for (const auto &card : all_cards) {
      if (card.suite == flush_suit) {
        flush_ranks.push_back(card.number);
      }
    }
    // Sort treating Ace (1) as 14 (highest)
    std::sort(flush_ranks.begin(), flush_ranks.end(), [](uint8_t a, uint8_t b) {
      uint8_t val_a = (a == 1) ? 14 : a;
      uint8_t val_b = (b == 1) ? 14 : b;
      return val_a > val_b; // Descending order
    });
  }

  // Check for straight flush
  auto check_straight = [](const std::vector<uint8_t> &ranks) -> int {
    if (ranks.empty())
      return 0;
    std::vector<uint8_t> sorted_ranks = ranks;
    std::sort(sorted_ranks.rbegin(), sorted_ranks.rend());

    // Remove duplicates
    sorted_ranks.erase(std::unique(sorted_ranks.begin(), sorted_ranks.end()),
                       sorted_ranks.end());

    bool has_ace = std::find(sorted_ranks.begin(), sorted_ranks.end(), 1) !=
                   sorted_ranks.end();

    // Check for 10-J-Q-K-A (Ace-high straight / Royal)
    if (has_ace) {
      bool is_ace_high = true;
      for (int rank : {10, 11, 12, 13}) {
        if (std::find(sorted_ranks.begin(), sorted_ranks.end(), rank) ==
            sorted_ranks.end()) {
          is_ace_high = false;
          break;
        }
      }
      if (is_ace_high)
        return 14; // Ace-high straight (return 14 to indicate Ace is high)
    }

    // Check for A-2-3-4-5 (wheel/low straight)
    if (has_ace && sorted_ranks.size() >= 4) {
      bool is_wheel = true;
      for (int i = 2; i <= 5; i++) {
        if (std::find(sorted_ranks.begin(), sorted_ranks.end(), i) ==
            sorted_ranks.end()) {
          is_wheel = false;
          break;
        }
      }
      if (is_wheel)
        return 5; // High card of wheel straight is 5
    }

    // Check for regular straights (high to low)
    for (size_t i = 0; i + 4 < sorted_ranks.size(); i++) {
      if (sorted_ranks[i] - sorted_ranks[i + 4] == 4) {
        return sorted_ranks[i]; // Return highest card
      }
    }
    return 0;
  };

  // Check straight flush
  if (!flush_ranks.empty()) {
    int straight_high = check_straight(flush_ranks);
    if (straight_high > 0) {
      HandValue result;
      if (straight_high == 14 || straight_high == 1) { // Ace high
        // Check if it's actually a royal flush (10-J-Q-K-A)
        bool is_royal = true;
        for (int rank : {10, 11, 12, 13, 1}) {
          if (std::find(flush_ranks.begin(), flush_ranks.end(), rank) ==
              flush_ranks.end()) {
            is_royal = false;
            break;
          }
        }
        result.rank =
            is_royal ? HandRank::ROYAL_FLUSH : HandRank::STRAIGHT_FLUSH;
      } else {
        result.rank = HandRank::STRAIGHT_FLUSH;
      }
      result.kickers = {static_cast<uint8_t>(straight_high), 0, 0, 0, 0};
      return result;
    }
  }

  // Find n-of-a-kinds and pairs
  std::vector<std::pair<int, uint8_t>> rank_groups; // (count, rank)
  for (int i = 1; i <= 13; i++) {
    if (rank_counts[i] > 0) {
      rank_groups.push_back({rank_counts[i], static_cast<uint8_t>(i)});
    }
  }

  // Sort by count (descending), then by rank (descending)
  std::sort(rank_groups.begin(), rank_groups.end(),
            [](const auto &a, const auto &b) {
              if (a.first != b.first)
                return a.first > b.first;
              // Ace (1) should be highest
              uint8_t rank_a = (a.second == 1) ? 14 : a.second;
              uint8_t rank_b = (b.second == 1) ? 14 : b.second;
              return rank_a > rank_b;
            });

  HandValue result;

  // Four of a kind
  if (rank_groups[0].first == 4) {
    result.rank = HandRank::FOUR_OF_A_KIND;
    uint8_t quad_rank =
        (rank_groups[0].second == 1) ? 14 : rank_groups[0].second;
    uint8_t kicker = (rank_groups[1].second == 1) ? 14 : rank_groups[1].second;
    result.kickers = {quad_rank, kicker, 0, 0, 0};
    return result;
  }

  // Full house
  if (rank_groups[0].first == 3 && rank_groups[1].first >= 2) {
    result.rank = HandRank::FULL_HOUSE;
    uint8_t trip_rank =
        (rank_groups[0].second == 1) ? 14 : rank_groups[0].second;
    uint8_t pair_rank =
        (rank_groups[1].second == 1) ? 14 : rank_groups[1].second;
    result.kickers = {trip_rank, pair_rank, 0, 0, 0};
    return result;
  }

  // Flush
  if (flush_suit != -1) {
    result.rank = HandRank::FLUSH;
    for (int i = 0; i < 5; i++) {
      result.kickers[i] = (flush_ranks[i] == 1) ? 14 : flush_ranks[i];
    }
    return result;
  }

  // Straight
  std::vector<uint8_t> all_ranks;
  for (const auto &card : all_cards) {
    all_ranks.push_back(card.number);
  }
  int straight_high = check_straight(all_ranks);
  if (straight_high > 0) {
    result.rank = HandRank::STRAIGHT;
    uint8_t high = (straight_high == 1) ? 14 : straight_high;
    result.kickers = {high, 0, 0, 0, 0};
    return result;
  }

  // Three of a kind
  if (rank_groups[0].first == 3) {
    result.rank = HandRank::THREE_OF_A_KIND;
    uint8_t trip_rank =
        (rank_groups[0].second == 1) ? 14 : rank_groups[0].second;
    uint8_t kicker1 = (rank_groups[1].second == 1) ? 14 : rank_groups[1].second;
    uint8_t kicker2 = (rank_groups[2].second == 1) ? 14 : rank_groups[2].second;
    result.kickers = {trip_rank, kicker1, kicker2, 0, 0};
    return result;
  }

  // Two pair
  if (rank_groups[0].first == 2 && rank_groups[1].first == 2) {
    result.rank = HandRank::TWO_PAIR;
    uint8_t pair1 = (rank_groups[0].second == 1) ? 14 : rank_groups[0].second;
    uint8_t pair2 = (rank_groups[1].second == 1) ? 14 : rank_groups[1].second;
    uint8_t kicker = (rank_groups[2].second == 1) ? 14 : rank_groups[2].second;
    result.kickers = {pair1, pair2, kicker, 0, 0};
    return result;
  }

  // One pair
  if (rank_groups[0].first == 2) {
    result.rank = HandRank::ONE_PAIR;
    uint8_t pair_rank =
        (rank_groups[0].second == 1) ? 14 : rank_groups[0].second;
    uint8_t kicker1 = (rank_groups[1].second == 1) ? 14 : rank_groups[1].second;
    uint8_t kicker2 = (rank_groups[2].second == 1) ? 14 : rank_groups[2].second;
    uint8_t kicker3 = (rank_groups[3].second == 1) ? 14 : rank_groups[3].second;
    result.kickers = {pair_rank, kicker1, kicker2, kicker3, 0};
    return result;
  }

  // High card
  result.rank = HandRank::HIGH_CARD;
  for (int i = 0; i < 5; i++) {
    result.kickers[i] =
        (rank_groups[i].second == 1) ? 14 : rank_groups[i].second;
  }
  return result;
}

// Structure to track side pots
struct Pot {
  int amount;
  std::vector<int> eligible_players;
};

template <int N>
std::vector<Pot> calculate_side_pots(const BoardState<N> &state) {
  std::vector<Pot> pots;

  // Create list of (player_idx, bet_amount, is_folded) for all players
  std::vector<std::tuple<int, int, bool>> player_bets;
  for (int i = 0; i < state.n_players; i++) {
    if (state.players[i].bet > 0) {
      bool is_folded = (state.players[i].state == PlayerState::State::FOLDED);
      player_bets.push_back({i, state.players[i].bet, is_folded});
    }
  }

  // Sort by bet amount
  std::sort(player_bets.begin(), player_bets.end(),
            [](const auto &a, const auto &b) {
              return std::get<1>(a) < std::get<1>(b);
            });

  int prev_bet_level = 0;
  std::vector<int>
      remaining_players; // All players contributing to current pot level
  std::vector<int> eligible_players; // Only non-folded players eligible to win

  for (const auto &pb : player_bets) {
    remaining_players.push_back(std::get<0>(pb));
    if (!std::get<2>(pb)) { // Not folded
      eligible_players.push_back(std::get<0>(pb));
    }
  }

  for (size_t i = 0; i < player_bets.size(); i++) {
    int current_bet_level = std::get<1>(player_bets[i]);

    if (current_bet_level > prev_bet_level) {
      Pot pot;
      pot.amount =
          (current_bet_level - prev_bet_level) * remaining_players.size();
      pot.eligible_players = eligible_players; // Only non-folded can win
      pots.push_back(pot);
      prev_bet_level = current_bet_level;
    }

    // Remove this player from both lists for next pot
    int player_idx = std::get<0>(player_bets[i]);
    remaining_players.erase(std::remove(remaining_players.begin(),
                                        remaining_players.end(), player_idx),
                            remaining_players.end());
    eligible_players.erase(std::remove(eligible_players.begin(),
                                       eligible_players.end(), player_idx),
                           eligible_players.end());
  }

  return pots;
}

template <int N> void finalise_showdown_impl(BoardState<N> &state) {
  // Calculate side pots
  std::vector<Pot> pots = calculate_side_pots(state);

  // Evaluate hands for all non-folded players
  std::array<HandValue, N> hand_values;
  for (int i = 0; i < state.n_players; i++) {
    if (state.players[i].state != PlayerState::State::FOLDED) {
      hand_values[i] = evaluate_hand(state.players[i].cards, state.openCards);
    }
  }

  // Initialize rewards to zero
  for (int i = 0; i < state.n_players; i++) {
    state.player_rewards[i] = 0;
  }

  // Distribute each pot
  for (const Pot &pot : pots) {
    // Find best hand among eligible players
    std::vector<int> winners;
    HandValue best_hand;
    bool first = true;

    for (int player_idx : pot.eligible_players) {
      if (first) {
        best_hand = hand_values[player_idx];
        winners.push_back(player_idx);
        first = false;
      } else {
        if (hand_values[player_idx] > best_hand) {
          best_hand = hand_values[player_idx];
          winners.clear();
          winners.push_back(player_idx);
        } else if (hand_values[player_idx] == best_hand) {
          winners.push_back(player_idx);
        }
      }
    }

    // Split pot among winners
    int pot_share = pot.amount / winners.size();
    int remainder = pot.amount % winners.size();

    for (size_t i = 0; i < winners.size(); i++) {
      int award = pot_share;
      if (i < static_cast<size_t>(remainder)) {
        award++; // Distribute remainder chips
      }
      state.player_rewards[winners[i]] += award;
      state.players[winners[i]].stack += award;
    }
  }

  // Reset bets for next hand
  for (int i = 0; i < state.n_players; i++) {
    state.players[i].bet = 0;
  }

  state.pot = 0;
}

// Explicit template instantiation for common sizes
template void finalise_showdown_impl<2>(BoardState<2> &);
template void finalise_showdown_impl<6>(BoardState<6> &);
template void finalise_showdown_impl<9>(BoardState<9> &);
template void finalise_showdown_impl<10>(BoardState<10> &);
