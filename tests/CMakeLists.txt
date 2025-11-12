# Find or fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test executables

# Board tests
add_executable(board_test test_board.cpp)
target_link_libraries(board_test GTest::gtest_main)
target_include_directories(board_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(board_comprehensive_test test_board_comprehensive.cpp)
target_link_libraries(board_comprehensive_test GTest::gtest_main)
target_include_directories(board_comprehensive_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Environment tests
add_executable(environment_test test_environment.cpp)
target_link_libraries(environment_test GTest::gtest_main)
target_include_directories(environment_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(environment_comprehensive_test test_environment_comprehensive.cpp)
target_link_libraries(environment_comprehensive_test GTest::gtest_main)
target_include_directories(environment_comprehensive_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Showdown tests
add_executable(showdown_test test_showdown.cpp)
target_link_libraries(showdown_test GTest::gtest_main)
target_include_directories(showdown_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(showdown_simple_test test_showdown_simple.cpp)
target_link_libraries(showdown_simple_test GTest::gtest_main)
target_include_directories(showdown_simple_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Register tests with labels
include(GoogleTest)
gtest_discover_tests(board_test TEST_PREFIX "board::")
gtest_discover_tests(board_comprehensive_test TEST_PREFIX "board::")
gtest_discover_tests(environment_test TEST_PREFIX "env::")
gtest_discover_tests(environment_comprehensive_test TEST_PREFIX "env::")
gtest_discover_tests(showdown_test TEST_PREFIX "showdown::")
gtest_discover_tests(showdown_simple_test TEST_PREFIX "showdown::")
