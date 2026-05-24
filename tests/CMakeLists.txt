include(GoogleTest)

# Test targets get NEITHER the Orthodoxy plugin NOR strict tidy (GoogleTest
# TEST() macros expand to classes/inheritance). They link the project library.

add_executable(game_test game_test.cc)
target_link_libraries(game_test PRIVATE minesweeper_lib gtest_main)
gtest_discover_tests(game_test)

add_executable(config_test config_test.cc)
target_link_libraries(config_test PRIVATE minesweeper_lib gtest_main)
gtest_discover_tests(config_test)

# Headless rendering regression guard (uses the source-tree assets).
add_executable(render_smoke_test render_smoke_test.cc)
target_link_libraries(render_smoke_test PRIVATE minesweeper_lib gtest_main)
target_compile_definitions(render_smoke_test PRIVATE
  ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
gtest_discover_tests(render_smoke_test)
