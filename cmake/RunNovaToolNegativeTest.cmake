cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED NOVA_COMPILE)
    message(FATAL_ERROR "NOVA_COMPILE is required")
endif()

if(NOT DEFINED TOOL_SOURCE)
    message(FATAL_ERROR "TOOL_SOURCE is required")
endif()

if(NOT DEFINED INPUT)
    message(FATAL_ERROR "INPUT is required")
endif()

if(NOT DEFINED EXPECT)
    message(FATAL_ERROR "EXPECT is required")
endif()

if(NOT DEFINED RUNTIME_DIR)
    message(FATAL_ERROR "RUNTIME_DIR is required")
endif()

if(NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "WORK_DIR is required")
endif()

if(NOT DEFINED CC)
    set(CC cc)
endif()

if(NOT DEFINED STRIP_RUNTIME_PREFIX)
    set(STRIP_RUNTIME_PREFIX OFF)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

get_filename_component(tool_name "${TOOL_SOURCE}" NAME_WE)
get_filename_component(input_name "${INPUT}" NAME_WE)
get_filename_component(expect_ext "${EXPECT}" EXT)

if(expect_ext STREQUAL "")
    set(expect_ext ".out")
endif()

set(generated_c "${WORK_DIR}/${tool_name}.c")
set(tool_exe "${WORK_DIR}/${tool_name}")
set(actual_output "${WORK_DIR}/${input_name}${expect_ext}")

function(normalize_text input_text output_var)
    set(text "${input_text}")
    string(REPLACE "\r\n" "\n" text "${text}")
    string(REPLACE "\r" "\n" text "${text}")
    string(REGEX REPLACE "\n+$" "" text "${text}")
    set(${output_var} "${text}" PARENT_SCOPE)
endfunction()

function(compare_text label actual expected)
    normalize_text("${actual}" actual_norm)
    normalize_text("${expected}" expected_norm)

    if(actual_norm STREQUAL expected_norm)
        return()
    endif()

    string(REPLACE "\n" ";" actual_lines "${actual_norm}")
    string(REPLACE "\n" ";" expected_lines "${expected_norm}")

    list(LENGTH actual_lines actual_count)
    list(LENGTH expected_lines expected_count)

    if(actual_count GREATER expected_count)
        set(max_count "${actual_count}")
    else()
        set(max_count "${expected_count}")
    endif()

    math(EXPR last_index "${max_count} - 1")

    if(last_index LESS 0)
        set(first_diff_line 1)
        set(actual_line "")
        set(expected_line "")
    else()
        foreach(i RANGE 0 ${last_index})
            if(i LESS actual_count)
                list(GET actual_lines ${i} actual_line_i)
            else()
                set(actual_line_i "<missing>")
            endif()

            if(i LESS expected_count)
                list(GET expected_lines ${i} expected_line_i)
            else()
                set(expected_line_i "<missing>")
            endif()

            if(NOT actual_line_i STREQUAL expected_line_i)
                math(EXPR first_diff_line "${i} + 1")
                set(actual_line "${actual_line_i}")
                set(expected_line "${expected_line_i}")
                break()
            endif()
        endforeach()
    endif()

    message(FATAL_ERROR
        "${label} mismatch at line ${first_diff_line}:\n"
        "  expected: ${expected_line}\n"
        "  actual  : ${actual_line}\n\n"
        "Full expected ${label}:\n${expected_norm}\n\n"
        "Full actual ${label}:\n${actual_norm}"
    )
endfunction()

message(STATUS "Compiling Nova tool: ${TOOL_SOURCE}")

execute_process(
    COMMAND "${NOVA_COMPILE}" "${TOOL_SOURCE}" "${generated_c}"
    RESULT_VARIABLE nova_compile_rv
    OUTPUT_VARIABLE nova_compile_out
    ERROR_VARIABLE nova_compile_err
)

if(NOT nova_compile_rv EQUAL 0)
    message(FATAL_ERROR
        "failed to compile Nova tool\n"
        "exit code: ${nova_compile_rv}\n"
        "stdout:\n${nova_compile_out}\n"
        "stderr:\n${nova_compile_err}"
    )
endif()

message(STATUS "Compiling generated C tool: ${generated_c}")

execute_process(
    COMMAND "${CC}"
            "${generated_c}"
            "${RUNTIME_DIR}/nova_runtime.c"
            "-I${RUNTIME_DIR}"
            "-std=c11"
            "-Wall"
            "-Wextra"
            "-o"
            "${tool_exe}"
    RESULT_VARIABLE cc_rv
    OUTPUT_VARIABLE cc_out
    ERROR_VARIABLE cc_err
)

if(NOT cc_rv EQUAL 0)
    message(FATAL_ERROR
        "failed to compile generated C tool\n"
        "exit code: ${cc_rv}\n"
        "stdout:\n${cc_out}\n"
        "stderr:\n${cc_err}"
    )
endif()

message(STATUS "Running negative Nova tool test: ${INPUT}")

execute_process(
    COMMAND "${tool_exe}" "${INPUT}" "${actual_output}"
    RESULT_VARIABLE run_rv
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)

if(run_rv EQUAL 0)
    message(FATAL_ERROR
        "expected generated Nova tool to fail, but it succeeded\n"
        "stdout:\n${run_out}\n"
        "stderr:\n${run_err}"
    )
endif()

file(READ "${EXPECT}" expected_err)
set(actual_err "${run_err}")

if(STRIP_RUNTIME_PREFIX)
    string(REGEX REPLACE "^Nova runtime error: " "" actual_err "${actual_err}")
endif()

compare_text("stderr" "${actual_err}" "${expected_err}")

message(STATUS "Negative tool test passed: ${INPUT}")