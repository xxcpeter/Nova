cmake_minimum_required(VERSION 3.16)

project(Nova LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(nova_lex
    src/token.cpp
    src/lexer.cpp
    src/nova_lex.cpp
)

target_include_directories(nova_lex
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nova_lex
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)


add_executable(nova_parse
    src/token.cpp
    src/lexer.cpp
    src/parser.cpp
    src/nova_parse.cpp
)

target_include_directories(nova_parse
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nova_parse
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)


add_executable(nova_sema
    src/token.cpp
    src/lexer.cpp
    src/parser.cpp
    src/sema.cpp
    src/nova_sema.cpp
)

target_include_directories(nova_sema
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nova_sema
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)


enable_testing()

set(TEXT_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunTextCompareTest.cmake)

function(add_lexer_positive_test name)
    add_test(
        NAME lexer_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_lex>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/positive/${name}.out
            -DMODE=positive
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(lexer_positive_${name} PROPERTIES LABELS "lexer;positive")
endfunction()

function(add_lexer_negative_test name)
    add_test(
        NAME lexer_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_lex>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/negative/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/negative/${name}.err
            -DMODE=negative
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(lexer_negative_${name} PROPERTIES LABELS "lexer;negative")
endfunction()

function(add_parser_positive_test name)
    add_test(
        NAME parser_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_parse>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/parser/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/parser/positive/${name}.out
            -DMODE=positive
            -DLABEL=stdout
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(parser_positive_${name} PROPERTIES LABELS "parser;positive")
endfunction()

function(add_parser_negative_test name)
    add_test(
        NAME parser_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_parse>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/parser/negative/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/parser/negative/${name}.err
            -DMODE=negative
            -DLABEL=stderr
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(parser_negative_${name} PROPERTIES LABELS "parser;negative")
endfunction()

function(add_sema_positive_test name)
    add_test(
        NAME sema_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_sema>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/sema/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/sema/positive/${name}.out
            -DMODE=positive
            -DLABEL=stdout
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(sema_positive_${name} PROPERTIES LABELS "sema;positive")
endfunction()

function(add_sema_negative_test name)
    add_test(
        NAME sema_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_sema>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/sema/negative/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/sema/negative/${name}.err
            -DMODE=negative
            -DLABEL=stderr
            -P ${TEXT_TEST_DRIVER}
    )
    set_tests_properties(sema_negative_${name} PROPERTIES LABELS "sema;negative")
endfunction()


add_lexer_positive_test(single_tokens)
add_lexer_positive_test(keywords_and_identifiers)
add_lexer_positive_test(numbers)
add_lexer_positive_test(strings)
add_lexer_positive_test(comments_and_whitespace)
add_lexer_positive_test(tiny_program)

add_lexer_negative_test(unclosed_string)
add_lexer_negative_test(dangling_escape_in_string)
add_lexer_negative_test(unclosed_block_comment)
add_lexer_negative_test(invalid_number_alpha)
add_lexer_negative_test(invalid_number_underscore)
add_lexer_negative_test(invalid_char)
add_lexer_negative_test(single_ampersand)
add_lexer_negative_test(single_pipe)

add_parser_positive_test(minimal_function)
add_parser_positive_test(let_and_return)
add_parser_positive_test(precedence)
add_parser_positive_test(assignment_right_assoc)
add_parser_positive_test(if_else)
add_parser_positive_test(while_loop)
add_parser_positive_test(function_call_and_forward_decl)
add_parser_positive_test(nested_blocks)

add_parser_negative_test(missing_semicolon)
add_parser_negative_test(missing_rparen)
add_parser_negative_test(missing_rbrace)
add_parser_negative_test(missing_colon_before_type)
add_parser_negative_test(missing_type_in_let)
add_parser_negative_test(invalid_return_expr)
add_parser_negative_test(incomplete_binary_expr)
add_parser_negative_test(malformed_call_args)

add_sema_positive_test(basic_program)
add_sema_positive_test(function_call)
add_sema_positive_test(shadowing)
add_sema_positive_test(recursion)
add_sema_positive_test(string_runtime)
add_sema_positive_test(all_paths_return)

add_sema_negative_test(undefined_variable)
add_sema_negative_test(duplicate_variable_same_scope)
add_sema_negative_test(duplicate_function)
add_sema_negative_test(duplicate_parameter_name)
add_sema_negative_test(assignment_type_mismatch)
add_sema_negative_test(invalid_assignment_target)
add_sema_negative_test(if_condition_not_bool)
add_sema_negative_test(while_condition_not_bool)
add_sema_negative_test(return_value_in_void_function)
add_sema_negative_test(empty_return_in_non_void_function)
add_sema_negative_test(missing_return_in_non_void_function)
add_sema_negative_test(return_type_mismatch)
add_sema_negative_test(function_argument_count_mismatch)
add_sema_negative_test(function_argument_type_mismatch)
add_sema_negative_test(call_unknown_function)