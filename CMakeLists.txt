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

enable_testing()

set(LEXER_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunLexerTest.cmake)

function(add_lexer_positive_test name)
    add_test(
        NAME lexer_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DLEXER=$<TARGET_FILE:nova_lex>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/positive/${name}.out
            -DMODE=positive
            -P ${LEXER_TEST_DRIVER}
    )
endfunction()

function(add_lexer_negative_test name)
    add_test(
        NAME lexer_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DLEXER=$<TARGET_FILE:nova_lex>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/negative/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/lexer/negative/${name}.err
            -DMODE=negative
            -P ${LEXER_TEST_DRIVER}
    )
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