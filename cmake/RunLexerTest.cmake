if(NOT DEFINED LEXER OR NOT DEFINED INPUT OR NOT DEFINED EXPECT OR NOT DEFINED MODE)
    message(FATAL_ERROR "LEXER / INPUT / EXPECT / MODE must all be defined")
endif()

execute_process(
    COMMAND "${LEXER}" "${INPUT}"
    RESULT_VARIABLE rv
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)

file(READ "${EXPECT}" expect)

# Normalize line endings first.
string(REPLACE "\r\n" "\n" out "${out}")
string(REPLACE "\r\n" "\n" err "${err}")
string(REPLACE "\r\n" "\n" expect "${expect}")

string(REPLACE "\r" "\n" out "${out}")
string(REPLACE "\r" "\n" err "${err}")
string(REPLACE "\r" "\n" expect "${expect}")

# Escape semicolons so CMake list splitting does not destroy token lines.
string(REPLACE ";" "\\;" out "${out}")
string(REPLACE ";" "\\;" err "${err}")
string(REPLACE ";" "\\;" expect "${expect}")

# Remove one trailing newline for cleaner comparison.
string(REGEX REPLACE "\n$" "" out "${out}")
string(REGEX REPLACE "\n$" "" err "${err}")
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

if(MODE STREQUAL "positive")
    if(NOT rv EQUAL 0)
        message(FATAL_ERROR
            "Expected success but got exit code ${rv}\n"
            "stdout:\n${out}\n"
            "stderr:\n${err}"
        )
    endif()

    compare_lines("stdout" "${out}" "${expect}")

elseif(MODE STREQUAL "negative")
    if(rv EQUAL 0)
        message(FATAL_ERROR
            "Expected failure but got exit code 0\n"
            "stdout:\n${out}\n"
            "stderr:\n${err}"
        )
    endif()

    compare_lines("stderr" "${err}" "${expect}")

else()
    message(FATAL_ERROR "Unknown MODE='${MODE}', expected positive or negative")
endif()