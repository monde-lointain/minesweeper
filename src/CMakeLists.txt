# Project library: every module except main.cc. Source manifest is frozen by
# Stream A; fan-out streams fill these files in but do not edit this list.
add_library(minesweeper_lib STATIC
  util/util.cc
  game/game.cc
  config/config.cc
  assets/assets.cc
  audio/audio.cc
  render/render.cc
  ui/ui.cc
  app/app.cc
)

target_include_directories(minesweeper_lib PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(minesweeper_lib PUBLIC
  SDL3::SDL3
  SDL3_mixer::SDL3_mixer
  imgui
  mini
  minesweeper_sanitizers  # PUBLIC: propagates -fsanitize to exe + tests, not deps
)

# Warnings on our code only (deps are SYSTEM / separate targets). Warnings-as-
# errors is applied below via the COMPILE_WARNING_AS_ERROR property, not a raw
# -Werror flag, so it never reaches dependency targets.
if(MSVC)
  set(MINESWEEPER_WARNINGS /W4)
else()
  set(MINESWEEPER_WARNINGS -Wall -Wextra -Wpedantic)
endif()
target_compile_options(minesweeper_lib PRIVATE ${MINESWEEPER_WARNINGS})

# Executable.
add_executable(minesweeper
  main.cc
)
target_compile_options(minesweeper PRIVATE ${MINESWEEPER_WARNINGS})
target_link_libraries(minesweeper PRIVATE minesweeper_lib)

# Warnings are errors for our targets only (always on; never dependencies).
set_target_properties(minesweeper_lib minesweeper
  PROPERTIES COMPILE_WARNING_AS_ERROR ON)

# Orthodox C++ enforcement: project targets only (never deps, never tests).
if(orthodoxy_plugin_FOUND)
  target_link_libraries(minesweeper_lib PRIVATE orthodoxy::plugin)
  target_link_libraries(minesweeper PRIVATE orthodoxy::plugin)
endif()

# Install assets next to the executable so SDL_GetBasePath()+"assets/" resolves.
add_custom_command(TARGET minesweeper POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:minesweeper>/assets
  COMMENT "Copying assets next to executable"
)
