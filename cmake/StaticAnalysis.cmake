# clang-tidy target (Orthodox-loosened .clang-tidy). Runs over our sources only.
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-22 clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
  file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cc
    ${CMAKE_SOURCE_DIR}/include/*.h
  )
  add_custom_target(tidy
    COMMAND ${CLANG_TIDY_EXECUTABLE}
      -p ${CMAKE_BINARY_DIR}
      ${ALL_CXX_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy"
  )
endif()
