#include "BoardPrinter.hpp"
#include "PokerEnvironment.cpp"
#include <iostream>
#include <sstream>
#include <string>

constexpr int MAX_PLAYERS = 6;
std::vector<std::pair<std::string, int>> starting_player_moneys = {
    {"p1", 100},
    {"p2", 100},
    {"p3", 100},
    {"p4", 100},
};

void printPlayerMap(
    const std::unordered_map<
        std::string, PokerEnvironment<MAX_PLAYERS>::PlayerInfo> &player_map) {
  std::cout << "\n╔════════════════════════════════════════════════════════════"
               "════╗\n";
  std::cout
      << "║                        PLAYER MAP                              ║\n";
  std::cout
      << "╠════════════════════════════════════════════════════════════════╣\n";

  if (player_map.empty()) {
    std::cout << "║  No players registered                                     "
                 "    ║\n";
  } else {
    for (const auto &[id, info] : player_map) {
      std::cout << "║ ID: " << std::left << std::setw(20) << id;
      std::cout << " Idx: " << std::setw(3) << info.idx;
      std::cout << " Active: " << std::setw(5) << (info.active ? "Yes" : "No");
      std::cout << " TopUp: $" << std::setw(10) << info.stack_topup << " ║\n";
    }
  }

  std::cout
      << "╚════════════════════════════════════════════════════════════════╝\n";
}

void printHelp() {
  std::cout << "\n╔════════════════════════════════════════════════════════════"
               "════╗\n";
  std::cout
      << "║                    POKER CLI COMMANDS                          ║\n";
  std::cout
      << "╠════════════════════════════════════════════════════════════════╣\n";
  std::cout
      << "║ upsert <player_id>                                             ║\n";
  std::cout
      << "║   - Add or reactivate a player                                 ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ topup <player_id> <amount>                                     ║\n";
  std::cout
      << "║   - Add chips to a player's stack                              ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ remove <player_id>                                             ║\n";
  std::cout
      << "║   - Remove or deactivate a player                              ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ reset_game                                                     ║\n";
  std::cout
      << "║   - Reset the entire game                                      ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ reset_hand                                                     ║\n";
  std::cout
      << "║   - Start a new hand                                           ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ step <player_id> <action> [amount]                             ║\n";
  std::cout
      << "║   - Take an action (fold, check, bet, call, raise, allin)      ║\n";
  std::cout
      << "║   - amount required for bet/raise                              ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ state                                                          ║\n";
  std::cout
      << "║   - Print current board state and player map                   ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ help                                                           ║\n";
  std::cout
      << "║   - Show this help message                                     ║\n";
  std::cout
      << "║                                                                ║\n";
  std::cout
      << "║ quit                                                           ║\n";
  std::cout
      << "║   - Exit the program                                           ║\n";
  std::cout
      << "╚════════════════════════════════════════════════════════════════╝\n";
}

PlayerAction parseAction(const std::string &action_str) {
  std::string lower = action_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == "fold")
    return PlayerAction::FOLD;
  if (lower == "check")
    return PlayerAction::CHECK;
  if (lower == "bet")
    return PlayerAction::BET;
  if (lower == "call")
    return PlayerAction::CALL;
  if (lower == "raise")
    return PlayerAction::RAISE;
  if (lower == "allin")
    return PlayerAction::ALL_IN;

  throw std::runtime_error("Invalid action: " + action_str);
}

int main() {
  std::print(
      "╔════════════════════════════════════════════════════════════════╗\n");
  std::print(
      "║           POKER ENVIRONMENT - INTERACTIVE CLI                  ║\n");
  std::print(
      "╚════════════════════════════════════════════════════════════════╝\n");
  std::print("Type 'help' for available commands\n\n");

  PokerEnvironment<MAX_PLAYERS> env;
  std::string line;
  bool has_state = false;

  for (auto &[id, money] : starting_player_moneys) {
    env.upsert_player(id);
    env.top_up_stack(id, money);
  }

  while (true) {
    std::cout << "> ";
    if (!std::getline(std::cin, line)) {
      break;
    }

    if (line.empty()) {
      continue;
    }

    std::istringstream iss(line);
    std::string command;
    iss >> command;

    try {
      if (command == "help") {
        printHelp();
      } else if (command == "quit" || command == "exit") {
        std::cout << "Exiting...\n";
        break;
      } else if (command == "upsert") {
        std::string player_id;
        if (!(iss >> player_id)) {
          std::cout << "Error: Usage: upsert <player_id>\n";
          continue;
        }
        env.upsert_player(player_id);
        std::cout << "Player '" << player_id << "' added/reactivated\n";
        printPlayerMap(env.get_player_map());
      } else if (command == "topup") {
        std::string player_id;
        int amount;
        if (!(iss >> player_id >> amount)) {
          std::cout << "Error: Usage: topup <player_id> <amount>\n";
          continue;
        }
        env.top_up_stack(player_id, amount);
        std::cout << "Player '" << player_id << "' topped up by $" << amount
                  << "\n";
        printPlayerMap(env.get_player_map());
      } else if (command == "remove") {
        std::string player_id;
        if (!(iss >> player_id)) {
          std::cout << "Error: Usage: remove <player_id>\n";
          continue;
        }
        env.remove_player(player_id);
        std::cout << "Player '" << player_id << "' removed/deactivated\n";
        printPlayerMap(env.get_player_map());
      } else if (command == "reset_game") {
        env.reset_game();
        has_state = false;
        std::cout << "Game reset\n";
        printPlayerMap(env.get_player_map());
      } else if (command == "reset_hand") {
        auto [state, left_players] = env.reset_hand();
        has_state = true;
        std::cout << "New hand started\n";
        if (!left_players.empty()) {
          std::cout << "\nPlayers who left:\n";
          for (const auto &[id, stack] : left_players) {
            std::cout << "  " << id << " left with $" << stack << "\n";
          }
        }
        printBoard(state);
        printPlayerMap(env.get_player_map());
      } else if (command == "step") {
        std::string player_id_str, action_str;
        int amount = 0;

        if (!(iss >> player_id_str >> action_str)) {
          std::cout << "Error: Usage: step <player_id> <action> [amount]\n";
          continue;
        }

        iss >> amount;

        PlayerAction action = parseAction(action_str);

        // Look up player in player_map
        const auto &player_map = env.get_player_map();
        auto it = player_map.find(player_id_str);
        if (it == player_map.end()) {
          std::cout << "Error: Player '" << player_id_str
                    << "' not found in player map\n";
          continue;
        }

        int player_idx = it->second.idx;
        if (player_idx < 0) {
          std::cout << "Error: Player '" << player_id_str
                    << "' has not been added to the board yet (call reset_hand "
                       "first)\n";
          continue;
        }

        Action poker_action{.player_id = static_cast<uint8_t>(player_idx),
                            .action = action,
                            .amount = amount};

        auto state = env.step(poker_action);
        has_state = true;
        std::cout << "Action executed\n";
        printBoard(state);
        printPlayerMap(env.get_player_map());
      } else if (command == "state") {
        if (has_state) {
          try {
            // Try to get current state through reset_hand if no hand in
            // progress Otherwise we need to track the state
            std::cout
                << "Note: Use 'reset_hand' or 'step' to see current state\n";
          } catch (...) {
            std::cout << "No active hand\n";
          }
        } else {
          std::cout << "No state available. Start a hand with 'reset_hand'\n";
        }
        printPlayerMap(env.get_player_map());
      } else {
        std::cout << "Unknown command: '" << command
                  << "'. Type 'help' for available commands.\n";
      }
    } catch (const std::exception &e) {
      std::cout << "Exception: " << e.what() << "\n";
      std::cout << "You can continue entering commands.\n";
    }
  }

  return 0;
}
