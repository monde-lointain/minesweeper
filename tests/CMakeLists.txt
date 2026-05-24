include(GoogleTest)

# Test targets get NEITHER the Orthodoxy plugin NOR strict tidy (GoogleTest
# TEST() macros expand to classes/inheritance). They link the project library.

add_executable(game_test game_test.cc)
target_link_libraries(game_test PRIVATE minesweeper_lib gtest_main)
gtest_discover_tests(game_test)

add_executable(config_test config_test.cc)
target_link_libraries(config_test PRIVATE minesweeper_lib gtest_main)
gtest_discover_tests(config_test)
