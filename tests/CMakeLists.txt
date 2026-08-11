# Copyright (c) 1992-2026 Paul McKibbin
function(queens_add_test name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE queens_core queens_warnings)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME ${name} COMMAND ${name} ${ARGN})
endfunction()

queens_add_test(test_board)
queens_add_test(test_options)
queens_add_test(test_genetic)
queens_add_test(test_backtrack ${CMAKE_CURRENT_SOURCE_DIR}/data/queens8.txt)

# The command-line surface, exercised through the real binary.
add_test(NAME cli_counts_eight_queens
         COMMAND queens --size 8 --count-only)
set_tests_properties(cli_counts_eight_queens PROPERTIES
                     PASS_REGULAR_EXPRESSION "^92 solutions for n=8")

add_test(NAME cli_rejects_bad_size COMMAND queens --size 0)
set_tests_properties(cli_rejects_bad_size PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_help COMMAND queens --help)
set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "usage:")
