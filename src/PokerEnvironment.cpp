#include "Board.cpp"
#include "PokerTypes.hpp"
#include <stdexcept>
#include <unordered_map>

template <int N> class PokerEnvironment {
public:
  PokerEnvironment() {
    static_assert(N >= 2, "At least 2 players required for Texas Hold'em");
  }

  void upsert_player(const std::string &id) {
    auto player = player_mp_.find(id);
    if (player != player_mp_.end()) {
      player->second.active = true;
    } else {
      if (player_mp_.size() >= N) {
        throw std::runtime_error("There are already the max number of players");
      }
      player_mp_[id] = PlayerInfo{.active = true};
    }
  }

  void top_up_stack(const std::string &id, const int amount) {
    auto player = player_mp_.find(id);
    if (player != player_mp_.end()) {
      if (!player->second.active) {
        throw std::runtime_error("Can't top up stack for inactive player");
      }
      player->second.stack_topup += amount;
    } else {
      throw std::runtime_error("Can't top up stack for non existent player");
    }
  }

  void remove_player(const std::string &id) {
    auto player = player_mp_.find(id);
    if (player != player_mp_.end()) {
      if (player->second.idx == -1) {
        player_mp_.erase(player);
      } else {
        player->second.active = false;
      }
    } else {
      throw std::runtime_error("Trying to remove a player that doesn't exists");
    }
  }

  void reset_game() {
    board_.hard_reset();
    player_mp_.clear();
  }

  std::pair<BoardState<N>, std::vector<std::pair<std::string, int>>>
  reset_hand() {
    if (board_.hand_in_progress()) {
      throw std::runtime_error("Can't reset hand when the game is still going");
    }

    // Check if we have enough active players
    int active_players = 0;
    for (const auto &[id, info] : player_mp_) {
      if (info.active) {
        active_players++;
      }
    }
    if (active_players < 2) {
      throw std::runtime_error("Need at least 2 players to start a hand");
    }

    // Convert map to vector for sorting
    std::vector<std::pair<std::string, PlayerInfo>> entries(player_mp_.begin(),
                                                            player_mp_.end());
    sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
      return a.second.idx < b.second.idx;
    });

    std::vector<std::pair<std::string, int>> left_players;

    // make sure that we keep the indexes aligned when we delete existing
    // players
    int n_deleted = 0;
    for (auto &[id, info] : entries) {
      if (info.idx == -1)
        continue;
      if (!info.active) {
        auto remove_player_state = board_.remove_player(info.idx - n_deleted);
        n_deleted++;
        player_mp_.erase(id);
        left_players.emplace_back(id, remove_player_state.stack);
      } else {
        player_mp_[id].idx -= n_deleted;
        board_.player_stack_topup(player_mp_[id].idx, info.stack_topup);
        player_mp_[id].stack_topup = 0;
      }
    }

    // now we add new players
    for (auto &[id, info] : entries) {
      if (info.idx == -1) {
        player_mp_[id].idx = board_.add_player(info.stack_topup);
        player_mp_[id].stack_topup = 0;
        if (!info.active) {
          throw std::runtime_error(
              "this should never happen, some bug occurred");
        }
      }
    }

    board_.reset_hand_random();
    board_.post_blinds();
    board_.start();

    return {board_.get_board_state(), left_players};
  }

  BoardState<N> step(Action action) {
    if (!board_.hand_in_progress()) {
      throw std::runtime_error(
          "No hand in progress. Call one of the reset_hand() first.");
    }
    try {
      board_.player_act(action);
      return board_.get_board_state();
    } catch (const std::runtime_error &e) {
      throw std::runtime_error(std::string("Action failed: ") + e.what());
    }
  }

  struct PlayerInfo {
    int idx = -1;
    bool active = true;
    int stack_topup = 0;
  };

  const std::unordered_map<std::string, PlayerInfo> &get_player_map() const {
    return player_mp_;
  }

private:
  PokerBoard<N> board_;
  std::unordered_map<std::string, PlayerInfo> player_mp_;
};
