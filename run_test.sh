#!/bin/bash

# Usage: ./run_test.sh [board|env|showdown|all]
# If no argument provided, runs all tests

TEST_SUITE=${1:-all}

cd build || { echo "Build directory not found. Run build_test.sh first."; exit 1; }

case "$TEST_SUITE" in
    board)
        echo "Running Board tests..."
        ctest -R "^board::" --output-on-failure
        ;;
    env)
        echo "Running Environment tests..."
        ctest -R "^env::" --output-on-failure
        ;;
    showdown)
        echo "Running Showdown tests..."
        ctest -R "^showdown::" --output-on-failure
        ;;
    all)
        echo "Running all tests..."
        ctest --output-on-failure
        ;;
    *)
        echo "Invalid test suite: $TEST_SUITE"
        echo "Usage: $0 [board|env|showdown|all]"
        echo ""
        echo "Options:"
        echo "  board    - Run all Board-related tests"
        echo "  env      - Run all Environment-related tests"
        echo "  showdown - Run all Showdown-related tests"
        echo "  all      - Run all tests (default)"
        exit 1
        ;;
esac

exit $?
