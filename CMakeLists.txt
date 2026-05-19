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


add_executable(nova_compile
    src/token.cpp
    src/lexer.cpp
    src/parser.cpp
    src/sema.cpp
    src/codegen_c.cpp
    src/nova_compile.cpp
)

target_include_directories(nova_compile
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nova_compile
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)


enable_testing()

set(TEXT_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunTextCompareTest.cmake)
set(CODEGEN_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunCodegenTest.cmake)
set(NOVA_TOOL_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunNovaToolTest.cmake)
set(NOVA_TOOL_NEGATIVE_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunNovaToolNegativeTest.cmake)

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

function(add_codegen_positive_test name)
    add_test(
        NAME codegen_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DCC=cc
            -P ${CODEGEN_TEST_DRIVER}
    )
    set_tests_properties(codegen_positive_${name} PROPERTIES LABELS "codegen;positive")
endfunction()

function(add_nova_lexer_tool_test name)
    add_test(
        NAME nova_tool_lexer_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_lexer.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/lexer/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/lexer/${name}.tok
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/lexer_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_lexer_${name} PROPERTIES LABELS "nova_tool;lexer")
endfunction()

function(add_nova_tokenizer_tool_test name)
    add_test(
        NAME nova_tool_tokenizer_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_tokenizer.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/tokenizer/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/tokenizer/${name}.tok
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/tokenizer_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_tokenizer_${name} PROPERTIES LABELS "nova_tool;tokenizer")
endfunction()

function(add_nova_parser_tool_test name)
    add_test(
        NAME nova_tool_parser_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_parser.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/parser/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/parser/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/parser_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_parser_${name} PROPERTIES LABELS "nova_tool;parser")
endfunction()

function(add_nova_checker_positive_test name)
    add_test(
        NAME nova_tool_checker_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_checker.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/checker/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/checker/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/checker_positive_${name}
            -DCC=cc
            -P ${NOVA_TOOL_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_checker_positive_${name} PROPERTIES LABELS "nova_tool;checker;positive")
endfunction()

function(add_nova_checker_negative_test name)
    add_test(
        NAME nova_tool_checker_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_checker.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/checker/negative/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/checker/negative/${name}.err
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/checker_negative_${name}
            -DSTRIP_RUNTIME_PREFIX=ON
            -DCC=cc
            -P ${NOVA_TOOL_NEGATIVE_TEST_DRIVER}
    )
    set_tests_properties(nova_tool_checker_negative_${name} PROPERTIES LABELS "nova_tool;checker;negative")
endfunction()


# step 2
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

# step 3
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

# step 4
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

# step 5
add_codegen_positive_test(helloworld)
add_codegen_positive_test(arithmetic)
add_codegen_positive_test(function_call)
add_codegen_positive_test(recursion)
add_codegen_positive_test(loop)
add_codegen_positive_test(string_concat)
add_codegen_positive_test(nested_block_shadowing)

# step 6
add_codegen_positive_test(string_len_get)
add_codegen_positive_test(string_slice)
add_codegen_positive_test(string_predicates)
add_codegen_positive_test(int_to_str_and_concat)
add_codegen_positive_test(buffer_builder)
add_codegen_positive_test(buffer_two_buffers)
add_codegen_positive_test(file_roundtrip)
add_codegen_positive_test(str_vec_basic)
add_codegen_positive_test(str_vec_two_vectors)
add_codegen_positive_test(str_vec_empty_len)
add_codegen_positive_test(str_vec_with_buffer)

# step 7
add_nova_lexer_tool_test(basic)
add_nova_lexer_tool_test(expressions)
add_nova_lexer_tool_test(strings)
add_nova_lexer_tool_test(comments)
add_nova_lexer_tool_test(operators)
add_nova_lexer_tool_test(block_comment_inline)

# step 8
add_codegen_positive_test(struct_basic)
add_codegen_positive_test(struct_assignment)
add_codegen_positive_test(struct_function_return)
add_codegen_positive_test(struct_nested)
add_codegen_positive_test(struct_field_order)

add_sema_negative_test(unknown_struct_type)
add_sema_negative_test(duplicate_struct)
add_sema_negative_test(duplicate_struct_field)
add_sema_negative_test(missing_struct_field)
add_sema_negative_test(unknown_struct_field)
add_sema_negative_test(duplicate_struct_literal_field)
add_sema_negative_test(field_type_mismatch)
add_sema_negative_test(access_field_on_non_struct)
add_sema_negative_test(unknown_field_access)
add_sema_negative_test(struct_field_void_type)
add_sema_negative_test(struct_param_unknown_type)

# step 9
add_codegen_positive_test(vec_int_basic)
add_codegen_positive_test(vec_struct_token)
add_codegen_positive_test(vec_set)
add_codegen_positive_test(vec_function_param)
add_codegen_positive_test(vec_function_return)
add_codegen_positive_test(vec_of_str)

add_sema_negative_test(vec_new_without_context)
add_sema_negative_test(vec_push_type_mismatch)
add_sema_negative_test(vec_get_index_not_int)
add_sema_negative_test(vec_len_on_non_vec)
add_sema_negative_test(vec_void_element)
add_sema_negative_test(vec_unknown_struct_element)

# step 10
add_nova_tokenizer_tool_test(basic)
add_nova_tokenizer_tool_test(struct_vec)
add_nova_tokenizer_tool_test(field_access)
add_nova_tokenizer_tool_test(strings_comments)

# step 11
add_nova_parser_tool_test(minimal_function)
add_nova_parser_tool_test(let_return)
add_nova_parser_tool_test(if_else)
add_nova_parser_tool_test(while_loop)
add_nova_parser_tool_test(struct_decl)
add_nova_parser_tool_test(vec_decl)
add_nova_parser_tool_test(nested_blocks)

# step 12
add_codegen_positive_test(enum_basic)
add_codegen_positive_test(enum_struct_field)
add_codegen_positive_test(enum_vec)

add_sema_negative_test(duplicate_enum)
add_sema_negative_test(duplicate_enum_member)
add_sema_negative_test(unknown_enum_member)
add_sema_negative_test(enum_type_mismatch)
add_sema_negative_test(enum_member_on_unknown_enum)

# step 13
add_nova_tokenizer_tool_test(enum_decl)
add_nova_parser_tool_test(enum_decl)

# step 14
add_nova_checker_positive_test(minimal_function)
add_nova_checker_positive_test(structs_enums_functions)
add_nova_checker_positive_test(recursive_parse_node_shape)
add_nova_checker_positive_test(local_shadow_nested_block)

add_nova_checker_negative_test(duplicate_function)
add_nova_checker_negative_test(duplicate_struct_field)
add_nova_checker_negative_test(duplicate_enum_member)
add_nova_checker_negative_test(unknown_field_type)
add_nova_checker_negative_test(void_field_type)
add_nova_checker_negative_test(direct_self_field)
add_nova_checker_negative_test(missing_main)
add_nova_checker_negative_test(invalid_main_return)
add_nova_checker_negative_test(duplicate_param)
add_nova_checker_negative_test(void_param)
add_nova_checker_negative_test(unknown_let_type)
add_nova_checker_negative_test(void_let_type)
add_nova_checker_negative_test(duplicate_let_same_block)