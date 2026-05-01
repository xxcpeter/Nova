if(NOT DEFINED NOVA_COMPILE OR NOT DEFINED INPUT OR NOT DEFINED EXPECT OR NOT DEFINED RUNTIME_DIR)
    message(FATAL_ERROR "NOVA_COMPILE / INPUT / EXPECT / RUNTIME_DIR must all be defined")
endif()

if(NOT DEFINED CC)
    set(CC cc)
endif()

# Make a per-test working directory under the CTest binary dir.
get_filename_component(test_name "${INPUT}" NAME_WE)
set(work_dir "${CMAKE_CURRENT_BINARY_DIR}/codegen_tests/${test_name}")
file(MAKE_DIRECTORY "${work_dir}")

set(generated_c "${work_dir}/${test_name}.c")
set(executable "${work_dir}/${test_name}_exe")

# 1. Run Nova compiler: .nv -> .c
execute_process(
    COMMAND "${NOVA_COMPILE}" "${INPUT}" "${generated_c}"
    RESULT_VARIABLE compile_rv
    OUTPUT_VARIABLE compile_out
    ERROR_VARIABLE compile_err
)

if(NOT compile_rv EQUAL 0)
    message(FATAL_ERROR
        "nova_compile failed with exit code ${compile_rv}\n"
        "stdout:\n${compile_out}\n"
        "stderr:\n${compile_err}"
    )
endif()

# 2. Compile generated C with runtime.
execute_process(
    COMMAND "${CC}"
        "${generated_c}"
        "${RUNTIME_DIR}/nova_runtime.c"
        "-I${RUNTIME_DIR}"
        -o "${executable}"
    RESULT_VARIABLE cc_rv
    OUTPUT_VARIABLE cc_out
    ERROR_VARIABLE cc_err
)

if(NOT cc_rv EQUAL 0)
    message(FATAL_ERROR
        "C compiler failed with exit code ${cc_rv}\n"
        "generated C: ${generated_c}\n"
        "stdout:\n${cc_out}\n"
        "stderr:\n${cc_err}"
    )
endif()

# 3. Run executable.
execute_process(
    COMMAND "${executable}"
    RESULT_VARIABLE run_rv
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)

if(NOT run_rv EQUAL 0)
    message(FATAL_ERROR
        "generated executable failed with exit code ${run_rv}\n"
        "stdout:\n${run_out}\n"
        "stderr:\n${run_err}"
    )
endif()

# 4. Read expected stdout.
file(READ "${EXPECT}" expect)

# Normalize line endings.
string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REPLACE "\r\n" "\n" expect "${expect}")

string(REPLACE "\r" "\n" run_out "${run_out}")
string(REPLACE "\r" "\n" expect "${expect}")

# Escape semicolons for CMake list handling.
string(REPLACE ";" "\\;" run_out "${run_out}")
string(REPLACE ";" "\\;" expect "${expect}")

# Remove one trailing newline.
string(REGEX REPLACE "\n$" "" run_out "${run_out}")
string(REGEX REPLACE "\n$" "" expect "${expect}")

function(compare_lines label actual expected)
    if("${actual}" STREQUAL "" AND "${expected}" STREQUAL "")
        return()
    endif()

    string(REPLACE "\n" ";" actual_lines "${actual}")
    string(REPLACE "\n" ";" expected_lines "${expected}")

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

compare_lines("stdout" "${run_out}" "${expect}")