# Texas Hold'em Poker Engine

A high-performance Texas Hold'em poker game engine written in modern C++23. This library provides a complete poker simulation environment with support for multiple players, comprehensive betting logic, hand evaluation, and pot distribution.

## Features

- **Full Texas Hold'em Implementation**: Complete poker game mechanics including all betting rounds (preflop, flop, turn, river, showdown)
- **Flexible Player Management**: Dynamic player addition, removal, and stack management
- **Sophisticated Pot Logic**: Proper side pot calculation for all-in situations
- **Hand Evaluation**: Accurate 7-card hand evaluation with support for all poker hand rankings
- **Interactive CLI**: Command-line interface for testing and demonstrations
- **Comprehensive Test Suite**: Extensive unit tests using Google Test framework
- **Template-Based Design**: Support for different player counts via C++ templates

## Getting Started

### Prerequisites

- CMake 3.25 or higher
- C++23 compatible compiler (GCC 13+, Clang 16+, or MSVC 2022+)
- Git (for downloading GoogleTest dependency)

### Building

Build the main executable:
```bash
./build_main.sh
```

Build with tests:
```bash
./build_test.sh
```

### Running

Start the interactive CLI:
```bash
./run_main.sh
```

Run the test suite:
```bash
./run_test.sh
```

## Interactive CLI Usage

The interactive CLI provides commands to manage players and simulate poker games:

### Commands

- `upsert <player_id>` - Add a new player or reactivate an inactive player
- `topup <player_id> <amount>` - Add chips to a player's stack
- `remove <player_id>` - Remove or deactivate a player
- `reset_game` - Reset the entire game state
- `reset_hand` - Start a new hand with current players
- `step <player_id> <action> [amount]` - Execute a player action
  - Actions: `fold`, `check`, `bet`, `call`, `raise`, `all_in`
  - Amount required for: `bet`, `raise`
- `state` - Display the current board state
- `help` - Show available commands
- `quit` - Exit the program

### Example Session

```bash
> upsert 0
Player 0 upserted

> upsert 1
Player 1 upserted

> topup 0 1000
Player 0 topped up with 1000

> topup 1 1000
Player 1 topped up with 1000

> reset_hand
Hand reset

> state
[Displays current game state with cards, bets, and pot]

> step 0 call 10
Player 0 called 10

> step 1 check
Player 1 checked
```

## Architecture

### Core Components

- **PokerTypes.hpp** - Core data structures and enums
  - `Card`: 8-bit card representation
  - `PlayerAction`: FOLD, CHECK, BET, CALL, RAISE, ALL_IN
  - `BoardState<N>`: Template structure for game state
  - `PlayerState`: Player information and status
  - `BettingRound`: SETUP, PREFLOP, FLOP, TURN, RIVER, SHOWDOWN

- **PokerEnvironment** - High-level game management API
  - Player lifecycle management
  - Stack management
  - Game and hand reset functionality
  - Action execution via `step()`

- **PokerBoard** - Core game engine
  - Deck management and card dealing
  - Betting round progression
  - Action validation and execution
  - Pot tracking

- **resolve_showdown** - Hand evaluation and pot distribution
  - 7-card hand evaluation
  - Side pot calculation
  - Winner determination
  - Chip distribution with proper tie handling

- **BoardPrinter** - Display utilities
  - Formatted game state output
  - Unicode card symbols (♣♦♥♠)

### Directory Structure

```
poker/
├── src/                          # Source code
│   ├── main.cpp                 # Interactive CLI
│   ├── PokerEnvironment.cpp     # Game management
│   ├── Board.cpp                # Core poker logic
│   ├── BoardPrinter.hpp         # Display utilities
│   ├── PokerTypes.hpp           # Data structures
│   └── resolve_showdown.cpp     # Hand evaluation
├── tests/                        # Test suite
│   ├── test_environment*.cpp    # Environment tests
│   ├── test_board*.cpp          # Board logic tests
│   └── test_showdown*.cpp       # Showdown tests
├── build/                        # Build artifacts
├── CMakeLists.txt               # Build configuration
├── build_main.sh                # Build script
├── build_test.sh                # Test build script
├── run_main.sh                  # Run CLI
└── run_test.sh                  # Run tests
```

## Hand Rankings

The engine supports all standard poker hand rankings:

1. Royal Flush
2. Straight Flush
3. Four of a Kind
4. Full House
5. Flush
6. Straight (including wheel: A-2-3-4-5)
7. Three of a Kind
8. Two Pair
9. One Pair
10. High Card

## Technical Details

### Template-Based Player Count

The game engine uses C++ templates to support different numbers of players:

```cpp
PokerBoard<6> board;  // 6-player game
PokerBoard<9> board;  // 9-player game
```

### Blind Logic

- **Heads-up (2 players)**: Small blind is on the button
- **Multi-way (3+ players)**: Standard blind positions

### Side Pot Handling

The engine correctly handles complex all-in scenarios with multiple side pots, ensuring proper pot distribution among eligible players.

### Hand Evaluation

Uses combinatorial evaluation of all possible 5-card combinations from the 7 available cards (2 hole cards + 5 community cards) to determine the best hand.

## Testing

The project includes comprehensive unit tests covering:

- Player management (upsert, topup, remove, reset)
- Betting logic and action validation
- Hand progression through all betting rounds
- Hand evaluation accuracy
- Pot distribution including side pots
- Edge cases and complex scenarios

Run tests with:
```bash
./run_test.sh
```

## Use Cases

- **Game Simulation**: Simulate poker games for analysis or entertainment
- **AI Training**: Environment for training poker AI agents
- **Education**: Learn poker rules and hand evaluation
- **Research**: Study game theory and poker strategies
- **Backend Engine**: Use as a poker game engine for applications

## License

This project is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome! Please ensure all tests pass before submitting changes:

```bash
./build_test.sh && ./run_test.sh
```

## Future Enhancements

Potential areas for expansion:
- Network multiplayer support
- Tournament mode with blind escalation
- Statistics and hand history tracking
- AI player implementations
- GUI interface
- Additional poker variants (Omaha, Stud, etc.)
