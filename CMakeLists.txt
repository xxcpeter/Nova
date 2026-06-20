cmake_minimum_required(VERSION 3.16)

project(Nova LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(nova_lex
    src/source_loader.cpp
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
    src/source_loader.cpp
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
    src/source_loader.cpp
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
    src/source_loader.cpp
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

set(NOVA_TEST_DRIVER_TEXT_COMPARE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunTextCompareTest.cmake)
set(NOVA_TEST_DRIVER_CODEGEN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunCodegenTest.cmake)
set(NOVA_TEST_DRIVER_TOOL ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunNovaToolTest.cmake)
set(NOVA_TEST_DRIVER_TOOL_CODEGEN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunNovaCodegenToolTest.cmake)
set(NOVA_TEST_DRIVER_TOOL_CLI ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunNovaToolCliTest.cmake)

function(add_nova_cpp_text_test component kind name ext stream)
    if(component STREQUAL "lexer")
        set(tool_target nova_lex)
    elseif(component STREQUAL "parser")
        set(tool_target nova_parse)
    elseif(component STREQUAL "sema")
        set(tool_target nova_sema)
    else()
        message(FATAL_ERROR "unknown C++ text test component: ${component}")
    endif()

    set(test_name ${component}_${kind}_${name})

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:${tool_target}>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/${component}/${kind}/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/${component}/${kind}/${name}.${ext}
            -DMODE=${kind}
            -DLABEL=${stream}
            -P ${NOVA_TEST_DRIVER_TEXT_COMPARE}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "${component};${kind}"
    )
endfunction()

function(add_lexer_positive_test name)
    add_nova_cpp_text_test(lexer positive ${name} out stdout)
endfunction()

function(add_lexer_negative_test name)
    add_nova_cpp_text_test(lexer negative ${name} err stderr)
endfunction()

function(add_parser_positive_test name)
    add_nova_cpp_text_test(parser positive ${name} out stdout)
endfunction()

function(add_parser_negative_test name)
    add_nova_cpp_text_test(parser negative ${name} err stderr)
endfunction()

function(add_sema_positive_test name)
    add_nova_cpp_text_test(sema positive ${name} out stdout)
endfunction()

function(add_sema_negative_test name)
    add_nova_cpp_text_test(sema negative ${name} err stderr)
endfunction()

function(add_nova_cpp_codegen_positive_test name)
    add_test(
        NAME codegen_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DTEST_NAME=codegen_positive_${name}
            -DCC=cc
            -P ${NOVA_TEST_DRIVER_CODEGEN}
    )
    set_tests_properties(codegen_positive_${name} PROPERTIES LABELS "codegen;positive")
endfunction()

function(add_nova_tool_test group kind tool name ext)
    set(test_name nova_tool_${group}_${kind}_${name})

    set(expect_failure OFF)
    set(strip_runtime_prefix OFF)

    if(kind STREQUAL "negative")
        set(expect_failure ON)
        set(strip_runtime_prefix ON)
    endif()

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/${group}/${kind}/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/${group}/${kind}/${name}.${ext}
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/${group}_${kind}_${name}
            -DCC=cc
            -DMODE=${kind}
            -DEXPECT_FAILURE=${expect_failure}
            -DSTRIP_RUNTIME_PREFIX=${strip_runtime_prefix}
            -P ${NOVA_TEST_DRIVER_TOOL}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;${group};${kind}"
    )
endfunction()

function(add_nova_tokenizer_positive_test name)
    add_nova_tool_test(tokenizer positive nova_tokenizer ${name} tok)
endfunction()

function(add_nova_parser_positive_test name)
    add_nova_tool_test(parser positive nova_parser ${name} out)
endfunction()

function(add_nova_parser_negative_test name)
    add_nova_tool_test(parser negative nova_parser ${name} err)
endfunction()

function(add_nova_checker_positive_test name)
    add_nova_tool_test(checker positive nova_checker ${name} out)
endfunction()

function(add_nova_checker_negative_test name)
    add_nova_tool_test(checker negative nova_checker ${name} err)
endfunction()

function(add_nova_typecheck_positive_test name)
    add_nova_tool_test(typecheck positive nova_checker ${name} out)
endfunction()

function(add_nova_typecheck_negative_test name)
    add_nova_tool_test(typecheck negative nova_checker ${name} err)
endfunction()

function(add_nova_frontend_test kind mode name ext)
    set(test_name nova_tool_frontend_${kind}_${mode}_${name})

    set(expect_failure OFF)
    set(strip_runtime_prefix OFF)

    if(kind STREQUAL "negative")
        set(expect_failure ON)
        set(strip_runtime_prefix ON)
    endif()

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/${kind}/${mode}/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/${kind}/${mode}/${name}.${ext}
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_${kind}_${mode}_${name}
            -DCC=cc
            -DMODE=${kind}
            -DTOOL_ARGS=${mode}
            -DEXPECT_FAILURE=${expect_failure}
            -DSTRIP_RUNTIME_PREFIX=${strip_runtime_prefix}
            -P ${NOVA_TEST_DRIVER_TOOL}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;frontend;${mode};${kind}"
    )
endfunction()

function(add_nova_frontend_positive_test mode name ext)
    add_nova_frontend_test(positive ${mode} ${name} ${ext})
endfunction()

function(add_nova_frontend_negative_test mode name)
    add_nova_frontend_test(negative ${mode} ${name} err)
endfunction()

function(add_nova_frontend_smoke_check name source_path)
    add_test(
        NAME nova_tool_frontend_smoke_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/${source_path}
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend/smoke/${name}.check
            -DEXPECT_PREFIX=ON
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_smoke_${name}
            -DCC=cc
            -DMODE=positive
            -DTOOL_ARGS=check
            -DEXPECT_FAILURE=OFF
            -P ${NOVA_TEST_DRIVER_TOOL}
    )

    set_tests_properties(
        nova_tool_frontend_smoke_${name}
        PROPERTIES LABELS "nova_tool;frontend;smoke"
    )
endfunction()

function(add_nova_frontend_import_test kind mode name ext)
    set(test_name nova_tool_frontend_import_${kind}_${mode}_${name})

    set(expect_failure OFF)
    set(strip_runtime_prefix OFF)

    if(kind STREQUAL "negative")
        set(expect_failure ON)
        set(strip_runtime_prefix ON)
    endif()

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_frontend.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend_import/${kind}/${mode}/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/frontend_import/${kind}/${mode}/${name}/main.${ext}
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/frontend_import_${kind}_${mode}_${name}
            -DCC=cc
            -DMODE=${kind}
            -DTOOL_ARGS=${mode}
            -DEXPECT_FAILURE=${expect_failure}
            -DSTRIP_RUNTIME_PREFIX=${strip_runtime_prefix}
            -P ${NOVA_TEST_DRIVER_TOOL}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;frontend;import;${mode};${kind}"
    )
endfunction()

function(add_nova_frontend_import_positive_test mode name ext)
    add_nova_frontend_import_test(positive ${mode} ${name} ${ext})
endfunction()

function(add_nova_frontend_import_negative_test mode name)
    add_nova_frontend_import_test(negative ${mode} ${name} err)
endfunction()

function(add_nova_codegen_tool_test name)
    set(test_name nova_tool_codegen_positive_${name})

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_codegen.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/codegen/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/codegen/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/codegen_positive_${name}
            -DCC=cc
            -P ${NOVA_TEST_DRIVER_TOOL_CODEGEN}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;codegen;positive"
    )
endfunction()

function(add_nova_import_codegen_positive_test name)
    add_test(
        NAME import_codegen_positive_${name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/positive/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/positive/${name}/main.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DTEST_NAME=import_codegen_positive_${name}
            -DCC=cc
            -P ${NOVA_TEST_DRIVER_CODEGEN}
    )
    set_tests_properties(import_codegen_positive_${name} PROPERTIES LABELS "import;codegen;positive")
endfunction()

function(add_nova_import_parse_negative_test name)
    add_test(
        NAME import_negative_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_parse>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/negative/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/negative/${name}/main.err
            -DMODE=negative
            -DLABEL=stderr
            -P ${NOVA_TEST_DRIVER_TEXT_COMPARE}
    )
    set_tests_properties(import_negative_${name} PROPERTIES LABELS "import;parser;negative")
endfunction()

function(add_nova_import_sema_negative_test name)
    add_test(
        NAME import_negative_sema_${name}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:nova_sema>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/negative/${name}/main.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/import/negative/${name}/main.err
            -DMODE=negative
            -DLABEL=stderr
            -P ${NOVA_TEST_DRIVER_TEXT_COMPARE}
    )
    set_tests_properties(
        import_negative_sema_${name} 
        PROPERTIES LABELS "import;sema;negative")
endfunction()

function(add_nova_selfhost_test)
    add_test(
        NAME self_host_stage_codegen
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/self_host.sh
    )

    set_tests_properties(
        self_host_stage_codegen
        PROPERTIES LABELS "selfhost;codegen"
    )
endfunction()

function(add_nova_compile_tool_test name)
    set(test_name nova_tool_compile_positive_${name})

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/nova_compile.nv
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/compile/positive/${name}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/compile/positive/${name}.out
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/compile_positive_${name}
            -DCC=cc
            -P ${NOVA_TEST_DRIVER_TOOL_CODEGEN}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;compile;positive"
    )
endfunction()

function(add_nova_tool_cli_negative_test group tool name)
    set(test_name nova_tool_cli_negative_${group}_${name})

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
            -DNOVA_COMPILE=$<TARGET_FILE:nova_compile>
            -DTOOL_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}.nv
            -DEXPECT=${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/cli/negative/${name}.err
            -DRUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/runtime
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/nova_tool_tests/cli_negative_${group}_${name}
            -DCC=cc
            -DCLI_ARGS=
            -DEXPECT_FAILURE=ON
            -DSTRIP_RUNTIME_PREFIX=ON
            -P ${NOVA_TEST_DRIVER_TOOL_CLI}
    )

    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "nova_tool;cli;negative;${group}"
    )
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
add_nova_cpp_codegen_positive_test(helloworld)
add_nova_cpp_codegen_positive_test(arithmetic)
add_nova_cpp_codegen_positive_test(function_call)
add_nova_cpp_codegen_positive_test(recursion)
add_nova_cpp_codegen_positive_test(loop)
add_nova_cpp_codegen_positive_test(string_concat)
add_nova_cpp_codegen_positive_test(nested_block_shadowing)

# step 6
add_nova_cpp_codegen_positive_test(string_len_get)
add_nova_cpp_codegen_positive_test(string_slice)
add_nova_cpp_codegen_positive_test(string_predicates)
add_nova_cpp_codegen_positive_test(int_to_str_and_concat)
add_nova_cpp_codegen_positive_test(buffer_builder)
add_nova_cpp_codegen_positive_test(buffer_two_buffers)
add_nova_cpp_codegen_positive_test(file_roundtrip)

# step 7
add_nova_tokenizer_positive_test(expressions)
add_nova_tokenizer_positive_test(strings)
add_nova_tokenizer_positive_test(comments)
add_nova_tokenizer_positive_test(operators)
add_nova_tokenizer_positive_test(block_comment_inline)

# step 8
add_nova_cpp_codegen_positive_test(struct_basic)
add_nova_cpp_codegen_positive_test(struct_assignment)
add_nova_cpp_codegen_positive_test(struct_function_return)
add_nova_cpp_codegen_positive_test(struct_nested)
add_nova_cpp_codegen_positive_test(struct_field_order)

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
add_nova_cpp_codegen_positive_test(vec_int_basic)
add_nova_cpp_codegen_positive_test(vec_struct_token)
add_nova_cpp_codegen_positive_test(vec_set)
add_nova_cpp_codegen_positive_test(vec_function_param)
add_nova_cpp_codegen_positive_test(vec_function_return)
add_nova_cpp_codegen_positive_test(vec_of_str)

add_sema_negative_test(vec_new_without_context)
add_sema_negative_test(vec_push_type_mismatch)
add_sema_negative_test(vec_get_index_not_int)
add_sema_negative_test(vec_len_on_non_vec)
add_sema_negative_test(vec_void_element)
add_sema_negative_test(vec_unknown_struct_element)

# step 10
add_nova_tokenizer_positive_test(basic)
add_nova_tokenizer_positive_test(struct_vec)
add_nova_tokenizer_positive_test(field_access)
add_nova_tokenizer_positive_test(strings_comments)

# step 11
add_nova_parser_positive_test(minimal_function)
add_nova_parser_positive_test(let_return)
add_nova_parser_positive_test(if_else)
add_nova_parser_positive_test(while_loop)
add_nova_parser_positive_test(struct_decl)
add_nova_parser_positive_test(vec_decl)
add_nova_parser_positive_test(nested_blocks)

# step 12
add_nova_cpp_codegen_positive_test(enum_basic)
add_nova_cpp_codegen_positive_test(enum_struct_field)
add_nova_cpp_codegen_positive_test(enum_vec)

add_sema_negative_test(duplicate_enum)
add_sema_negative_test(duplicate_enum_member)
add_sema_negative_test(unknown_enum_member)
add_sema_negative_test(enum_type_mismatch)
add_sema_negative_test(enum_member_on_unknown_enum)

# step 13
add_nova_tokenizer_positive_test(enum_decl)
add_nova_parser_positive_test(enum_decl)

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

# step 15
add_nova_parser_positive_test(expression_precedence)
add_nova_parser_positive_test(expression_call_field)
add_nova_parser_positive_test(expression_struct_literal)
add_nova_parser_positive_test(expression_assignment)
add_nova_parser_positive_test(condition_expr)
add_nova_parser_positive_test(expression_return_value)
add_nova_parser_positive_test(expression_unary)

add_nova_parser_negative_test(missing_expr_after_assign)
add_nova_parser_negative_test(missing_rparen_in_call)
add_nova_parser_negative_test(malformed_struct_literal)
add_nova_parser_negative_test(incomplete_binary_expr)
add_nova_parser_negative_test(missing_rbrace)
add_nova_parser_negative_test(missing_semicolon)

# step 16
add_nova_typecheck_positive_test(let_return_int)
add_nova_typecheck_positive_test(struct_literal_field_access)
add_nova_typecheck_positive_test(enum_member_compare)
add_nova_typecheck_positive_test(vec_basic)
add_nova_typecheck_positive_test(while_assignment)

add_nova_typecheck_negative_test(let_type_mismatch)
add_nova_typecheck_negative_test(return_type_mismatch)
add_nova_typecheck_negative_test(if_condition_not_bool)
add_nova_typecheck_negative_test(undefined_variable)
add_nova_typecheck_negative_test(call_arg_mismatch)
add_nova_typecheck_negative_test(unknown_field)
add_nova_typecheck_negative_test(unknown_enum_member)
add_nova_typecheck_negative_test(vec_new_without_expected_type)
add_nova_typecheck_negative_test(vec_push_type_mismatch)
add_nova_typecheck_negative_test(assignment_type_mismatch)
add_nova_typecheck_negative_test(non_assignable_target)

# step 17
add_nova_frontend_positive_test(tokens basic tok)
add_nova_frontend_positive_test(tokens enum_expr tok)

add_nova_frontend_positive_test(parse expression out)
add_nova_frontend_positive_test(parse condition out)

add_nova_frontend_positive_test(check let_return_int check)
add_nova_frontend_positive_test(check struct_enum_vec check)

add_nova_frontend_negative_test(check type_mismatch)
add_nova_frontend_negative_test(check undefined_variable)
add_nova_frontend_negative_test(parse missing_expr)

add_nova_frontend_smoke_check(nova_tokenizer tools/nova_tokenizer.nv)
add_nova_frontend_smoke_check(nova_parser tools/nova_parser.nv)
add_nova_frontend_smoke_check(nova_checker tools/nova_checker.nv)

# step 18
add_nova_import_codegen_positive_test(simple)
add_nova_import_codegen_positive_test(diamond)

add_nova_import_parse_negative_test(missing)
add_nova_import_parse_negative_test(cyclic)

add_nova_frontend_import_positive_test(check simple check)

# step 19
add_nova_codegen_tool_test(hello)
add_nova_codegen_tool_test(arithmetic)
add_nova_codegen_tool_test(function_call)
add_nova_codegen_tool_test(if_else)
add_nova_codegen_tool_test(else_if_chain)
add_nova_codegen_tool_test(control_flow_nested)
add_nova_codegen_tool_test(while_loop)
add_nova_codegen_tool_test(bool_logic)
add_nova_codegen_tool_test(string_concat)
add_nova_codegen_tool_test(recursion)

add_nova_codegen_tool_test(string_len_get)
add_nova_codegen_tool_test(string_slice_predicates)
add_nova_codegen_tool_test(int_to_str_concat)
add_nova_codegen_tool_test(buffer_builder)

add_nova_codegen_tool_test(enum_basic)
add_nova_codegen_tool_test(enum_function_param)

add_nova_codegen_tool_test(struct_basic)
add_nova_codegen_tool_test(struct_function_return)
add_nova_codegen_tool_test(struct_assignment)
add_nova_codegen_tool_test(struct_nested)

add_nova_codegen_tool_test(vec_int_basic)
add_nova_codegen_tool_test(vec_set_int)
add_nova_codegen_tool_test(vec_struct_basic)
add_nova_codegen_tool_test(vec_param_get)
add_nova_codegen_tool_test(vec_return_new)
add_nova_codegen_tool_test(struct_return_literal)
add_nova_codegen_tool_test(vec_str_basic)
add_nova_codegen_tool_test(vec_str_param)
add_nova_codegen_tool_test(vec_struct_param)
add_nova_codegen_tool_test(vec_shadowing)

# step 22
add_nova_cpp_codegen_positive_test(string_plus)
add_nova_typecheck_positive_test(string_plus)
add_nova_codegen_tool_test(string_plus)

add_nova_typecheck_positive_test(noreturn_error_call)
add_nova_typecheck_positive_test(noreturn_if_else)
add_nova_typecheck_negative_test(missing_return)

# step 23
add_nova_import_parse_negative_test(source_location_parse)
add_nova_import_sema_negative_test(source_location_sema)
add_nova_frontend_import_negative_test(check source_location)
add_nova_frontend_negative_test(parse eof_missing_rbrace)

# step 24
add_nova_import_codegen_positive_test(duplicate_same_file)
add_nova_import_codegen_positive_test(duplicate_parent_path)
add_nova_import_codegen_positive_test(nested_relative)
add_nova_import_codegen_positive_test(import_in_middle)

add_nova_cpp_codegen_positive_test(string_ends_with)
add_nova_typecheck_positive_test(string_ends_with)
add_nova_codegen_tool_test(string_ends_with)

# step 25
add_nova_selfhost_test()

# step 28
add_nova_compile_tool_test(hello)
add_nova_compile_tool_test(struct_vec)
add_nova_compile_tool_test(recursion)

add_nova_tool_cli_negative_test(frontend nova_frontend nova_frontend_bad_args)
add_nova_tool_cli_negative_test(codegen nova_codegen nova_codegen_bad_args)
add_nova_tool_cli_negative_test(compile nova_compile nova_compile_bad_args)
add_nova_tool_cli_negative_test(tokenizer nova_tokenizer nova_tokenizer_bad_args)
add_nova_tool_cli_negative_test(parser nova_parser nova_parser_bad_args)
add_nova_tool_cli_negative_test(checker nova_checker nova_checker_bad_args)

# step 29
add_nova_cpp_codegen_positive_test(runtime_process_filesystem)
add_nova_codegen_tool_test(runtime_process_filesystem)
add_nova_typecheck_positive_test(runtime_process_filesystem)

add_nova_typecheck_positive_test(nova_build_cli)
add_nova_tool_cli_negative_test(build nova_build nova_build_bad_args)