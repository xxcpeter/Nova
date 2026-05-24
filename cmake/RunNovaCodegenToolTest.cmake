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

if(NOT EXISTS "${TOOL_SOURCE}")
    message(FATAL_ERROR "TOOL_SOURCE does not exist: ${TOOL_SOURCE}")
endif()

if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "INPUT does not exist: ${INPUT}")
endif()

if(NOT EXISTS "${EXPECT}")
    message(FATAL_ERROR "EXPECT does not exist: ${EXPECT}")
endif()

if(NOT EXISTS "${RUNTIME_DIR}/nova_runtime.c")
    message(FATAL_ERROR "runtime source does not exist: ${RUNTIME_DIR}/nova_runtime.c")
endif()

if(NOT EXISTS "${RUNTIME_DIR}/nova_runtime.h")
    message(FATAL_ERROR "runtime header does not exist: ${RUNTIME_DIR}/nova_runtime.h")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

get_filename_component(tool_name "${TOOL_SOURCE}" NAME_WE)
get_filename_component(input_name "${INPUT}" NAME_WE)

set(tool_c "${WORK_DIR}/${tool_name}.c")
set(tool_exe "${WORK_DIR}/${tool_name}_exe")

set(generated_c "${WORK_DIR}/${input_name}.generated.c")
set(generated_exe "${WORK_DIR}/${input_name}_generated_exe")

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

message(STATUS "Compiling Nova codegen tool: ${TOOL_SOURCE}")

execute_process(
    COMMAND "${NOVA_COMPILE}" "${TOOL_SOURCE}" "${tool_c}"
    RESULT_VARIABLE nova_compile_rv
    OUTPUT_VARIABLE nova_compile_out
    ERROR_VARIABLE nova_compile_err
)

if(NOT nova_compile_rv EQUAL 0)
    message(FATAL_ERROR
        "failed to compile Nova codegen tool\n"
        "tool source: ${TOOL_SOURCE}\n"
        "exit code: ${nova_compile_rv}\n"
        "stdout:\n${nova_compile_out}\n"
        "stderr:\n${nova_compile_err}"
    )
endif()

message(STATUS "Compiling generated C codegen tool: ${tool_c}")

execute_process(
    COMMAND "${CC}"
            "${tool_c}"
            "${RUNTIME_DIR}/nova_runtime.c"
            "-I${RUNTIME_DIR}"
            "-std=c11"
            "-Wall"
            "-Wextra"
            "-o"
            "${tool_exe}"
    RESULT_VARIABLE tool_cc_rv
    OUTPUT_VARIABLE tool_cc_out
    ERROR_VARIABLE tool_cc_err
)

if(NOT tool_cc_rv EQUAL 0)
    message(FATAL_ERROR
        "failed to compile generated C codegen tool\n"
        "generated tool c: ${tool_c}\n"
        "exit code: ${tool_cc_rv}\n"
        "stdout:\n${tool_cc_out}\n"
        "stderr:\n${tool_cc_err}"
    )
endif()

message(STATUS "Running Nova codegen tool: ${INPUT}")

execute_process(
    COMMAND "${tool_exe}" "${INPUT}" "${generated_c}"
    RESULT_VARIABLE codegen_rv
    OUTPUT_VARIABLE codegen_out
    ERROR_VARIABLE codegen_err
)

if(NOT codegen_rv EQUAL 0)
    message(FATAL_ERROR
        "nova_codegen tool failed\n"
        "tool: ${tool_exe}\n"
        "input: ${INPUT}\n"
        "generated c: ${generated_c}\n"
        "exit code: ${codegen_rv}\n"
        "stdout:\n${codegen_out}\n"
        "stderr:\n${codegen_err}"
    )
endif()

if(NOT EXISTS "${generated_c}")
    message(FATAL_ERROR "nova_codegen did not create generated C file: ${generated_c}")
endif()

message(STATUS "Compiling generated C program: ${generated_c}")

execute_process(
    COMMAND "${CC}"
            "${generated_c}"
            "${RUNTIME_DIR}/nova_runtime.c"
            "-I${RUNTIME_DIR}"
            "-std=c11"
            "-Wall"
            "-Wextra"
            "-o"
            "${generated_exe}"
    RESULT_VARIABLE generated_cc_rv
    OUTPUT_VARIABLE generated_cc_out
    ERROR_VARIABLE generated_cc_err
)

if(NOT generated_cc_rv EQUAL 0)
    file(READ "${generated_c}" generated_source)
    message(FATAL_ERROR
        "failed to compile generated C program\n"
        "generated c: ${generated_c}\n"
        "exit code: ${generated_cc_rv}\n"
        "stdout:\n${generated_cc_out}\n"
        "stderr:\n${generated_cc_err}\n\n"
        "Generated C source:\n${generated_source}"
    )
endif()

message(STATUS "Running generated C program")

execute_process(
    COMMAND "${generated_exe}"
    RESULT_VARIABLE run_rv
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)

if(NOT run_rv EQUAL 0)
    file(READ "${generated_c}" generated_source)
    message(FATAL_ERROR
        "generated C program failed\n"
        "executable: ${generated_exe}\n"
        "exit code: ${run_rv}\n"
        "stdout:\n${run_out}\n"
        "stderr:\n${run_err}\n\n"
        "Generated C source:\n${generated_source}"
    )
endif()

file(READ "${EXPECT}" expected_out)

compare_text("stdout" "${run_out}" "${expected_out}")

message(STATUS "Nova codegen tool test passed: ${INPUT}")