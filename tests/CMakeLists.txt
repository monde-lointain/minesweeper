include(GoogleTest)

# Test targets get NEITHER the Orthodoxy plugin NOR strict tidy (GoogleTest
# TEST() macros expand to classes/inheritance). They link the project library.

add_executable(game_test game_test.cc)
target_link_libraries(game_test PRIVATE minesweeper_lib gtest_main)

add_executable(config_test config_test.cc)
target_link_libraries(config_test PRIVATE minesweeper_lib gtest_main)

# Headless rendering regression guard (uses the source-tree assets).
add_executable(render_smoke_test render_smoke_test.cc)
target_link_libraries(render_smoke_test PRIVATE minesweeper_lib gtest_main)
target_compile_definitions(render_smoke_test PRIVATE
  ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")

# Windows: each test exe links SDL3.dll / SDL3_mixer.dll. Stage those runtime
# DLLs next to the exe BEFORE gtest_discover_tests launches it to enumerate
# cases, else the exe can't start (exit 0xc0000135, DLL-not-found). The
# generator expression is empty on Linux/macOS, so this is Windows-only.
if(WIN32)
  foreach(t game_test config_test render_smoke_test)
    add_custom_command(TARGET ${t} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${t}>
        $<TARGET_RUNTIME_DLLS:${t}>
      COMMAND_EXPAND_LISTS)
  endforeach()
endif()

gtest_discover_tests(game_test)
gtest_discover_tests(config_test)
gtest_discover_tests(render_smoke_test)
