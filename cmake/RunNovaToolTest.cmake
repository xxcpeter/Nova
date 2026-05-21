if(NOT EXISTS "${TOOL_SOURCE}")
    message(FATAL_ERROR "TOOL_SOURCE does not exist: ${TOOL_SOURCE}")
endif()

if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "INPUT does not exist: ${INPUT}")
endif()

if(NOT EXISTS "${EXPECT}")
    message(FATAL_ERROR "EXPECT does not exist: ${EXPECT}")
endif()

if(NOT DEFINED NOVA_COMPILE OR
   NOT DEFINED TOOL_SOURCE OR
   NOT DEFINED INPUT OR
   NOT DEFINED EXPECT OR
   NOT DEFINED RUNTIME_DIR OR
   NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "NOVA_COMPILE / TOOL_SOURCE / INPUT / EXPECT / RUNTIME_DIR / WORK_DIR must all be defined")
endif()

if(NOT DEFINED CC)
    set(CC cc)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

get_filename_component(tool_name "${TOOL_SOURCE}" NAME_WE)
get_filename_component(input_name "${INPUT}" NAME_WE)

set(generated_c "${WORK_DIR}/${tool_name}.c")
set(tool_exe "${WORK_DIR}/${tool_name}_exe")

get_filename_component(expect_ext "${EXPECT}" EXT)
if(expect_ext STREQUAL "")
    set(expect_ext ".out")
endif()

set(actual_output "${WORK_DIR}/${input_name}${expect_ext}")

# 1. Compile Nova tool source to C.
execute_process(
    COMMAND "${NOVA_COMPILE}" "${TOOL_SOURCE}" "${generated_c}"
    RESULT_VARIABLE compile_rv
    OUTPUT_VARIABLE compile_out
    ERROR_VARIABLE compile_err
)

if(NOT compile_rv EQUAL 0)
    message(FATAL_ERROR
        "nova_compile failed while compiling tool\n"
        "tool source: ${TOOL_SOURCE}\n"
        "exit code: ${compile_rv}\n"
        "stdout:\n${compile_out}\n"
        "stderr:\n${compile_err}"
    )
endif()

# 2. Compile generated C tool with Nova runtime.
execute_process(
    COMMAND "${CC}"
        "${generated_c}"
        "${RUNTIME_DIR}/nova_runtime.c"
        "-I${RUNTIME_DIR}"
        -o "${tool_exe}"
    RESULT_VARIABLE cc_rv
    OUTPUT_VARIABLE cc_out
    ERROR_VARIABLE cc_err
)

if(NOT cc_rv EQUAL 0)
    message(FATAL_ERROR
        "C compiler failed while compiling generated Nova tool\n"
        "generated C: ${generated_c}\n"
        "exit code: ${cc_rv}\n"
        "stdout:\n${cc_out}\n"
        "stderr:\n${cc_err}"
    )
endif()

# 3. Run generated Nova tool.
if(DEFINED TOOL_ARGS)
    separate_arguments(extra_args UNIX_COMMAND "${TOOL_ARGS}")
else()
    set(extra_args "")
endif()

execute_process(
    COMMAND "${tool_exe}" ${extra_args} "${INPUT}" "${actual_output}"
    RESULT_VARIABLE run_rv
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)

if(NOT run_rv EQUAL 0)
    message(FATAL_ERROR
        "generated Nova tool failed\n"
        "tool: ${tool_exe}\n"
        "input: ${INPUT}\n"
        "output: ${actual_output}\n"
        "exit code: ${run_rv}\n"
        "stdout:\n${run_out}\n"
        "stderr:\n${run_err}"
    )
endif()

if(NOT EXISTS "${actual_output}")
    message(FATAL_ERROR
        "generated Nova tool did not create expected output file\n"
        "expected output file: ${actual_output}\n"
        "stdout:\n${run_out}\n"
        "stderr:\n${run_err}"
    )
endif()

file(READ "${actual_output}" actual)
file(READ "${EXPECT}" expected)

# Normalize line endings.
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")

string(REPLACE "\r" "\n" actual "${actual}")
string(REPLACE "\r" "\n" expected "${expected}")

# Escape semicolons so CMake list splitting does not break lines.
string(REPLACE ";" "\\;" actual "${actual}")
string(REPLACE ";" "\\;" expected "${expected}")

# Remove one trailing newline.
string(REGEX REPLACE "\n$" "" actual "${actual}")
string(REGEX REPLACE "\n$" "" expected "${expected}")

function(compare_lines label actual_text expected_text)
    if("${actual_text}" STREQUAL "" AND "${expected_text}" STREQUAL "")
        return()
    endif()

    string(REPLACE "\n" ";" actual_lines "${actual_text}")
    string(REPLACE "\n" ";" expected_lines "${expected_text}")

    list(LENGTH actual_lines actual_len)
    list(LENGTH expected_lines expected_len)

    if(actual_len GREATER expected_len)
        set(max_len ${actual_len})
    else()
        set(max_len ${expected_len})
    endif()

    if(max_len EQUAL 0)
        return()
    endif()

    math(EXPR last_index "${max_len} - 1")
    set(found_diff FALSE)

    foreach(i RANGE 0 ${last_index})
        if(i LESS actual_len)
            list(GET actual_lines ${i} actual_line)
        else()
            set(actual_line "<missing>")
        endif()

        if(i LESS expected_len)
            list(GET expected_lines ${i} expected_line)
        else()
            set(expected_line "<missing>")
        endif()

        if(NOT actual_line STREQUAL expected_line)
            math(EXPR line_no "${i} + 1")
            message(STATUS "${label} mismatch at line ${line_no}:")
            message(STATUS "  expected: ${expected_line}")
            message(STATUS "  actual  : ${actual_line}")
            set(found_diff TRUE)
        endif()
    endforeach()

    if(found_diff)
        message(FATAL_ERROR "${label} did not match expected output")
    endif()
endfunction()

compare_lines("tool output" "${actual}" "${expected}")