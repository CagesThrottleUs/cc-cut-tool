cmake_minimum_required(VERSION 4.0.0)

# ---------------------------------------------------------------------------
# Integration-test prerequisites
# ---------------------------------------------------------------------------

find_program(BASH_EXECUTABLE  bash)
find_program(PERL_EXECUTABLE  perl)
find_program(TAIL_EXECUTABLE  tail)

set(INTEG_DIR   ${CMAKE_CURRENT_SOURCE_DIR})
set(SAMPLE_DIR  ${CMAKE_SOURCE_DIR}/sample)
set(CUT_TARGET  ${PROJECT_NAME})

# ---------------------------------------------------------------------------
# Helper macro — attach common properties to every integration test
# ---------------------------------------------------------------------------

macro(add_integration_test name)
    set_tests_properties(${name} PROPERTIES
        LABELS   "integration"
        TIMEOUT  60
    )
endmacro()

# ---------------------------------------------------------------------------
# 1.  User test cases  (the 8 scenarios from the specification)
# ---------------------------------------------------------------------------

if(BASH_EXECUTABLE AND TAIL_EXECUTABLE)
    add_test(
        NAME    integ_user_cases
        COMMAND ${BASH_EXECUTABLE}
                    ${INTEG_DIR}/test_user_cases.sh
                    $<TARGET_FILE:${CUT_TARGET}>
                    ${SAMPLE_DIR}
    )
    add_integration_test(integ_user_cases)
    set_tests_properties(integ_user_cases PROPERTIES
        LABELS "integration;user"
    )
else()
    message(WARNING
        "Integration: bash or tail not found — integ_user_cases will not be registered")
endif()

# ---------------------------------------------------------------------------
# 2.  Byte-mode acceptance tests  (TC-REQ007)
# ---------------------------------------------------------------------------

if(BASH_EXECUTABLE)
    add_test(
        NAME    integ_byte_mode
        COMMAND ${BASH_EXECUTABLE}
                    ${INTEG_DIR}/test_byte_mode.sh
                    $<TARGET_FILE:${CUT_TARGET}>
    )
    add_integration_test(integ_byte_mode)
    set_tests_properties(integ_byte_mode PROPERTIES
        LABELS "integration;byte_mode"
    )
else()
    message(WARNING
        "Integration: bash not found — integ_byte_mode will not be registered")
endif()

# ---------------------------------------------------------------------------
# 3.  Coreutils cut.pl test vectors
#     run_cut_tests.pl provides the test framework and filters out tests
#     for unsupported features (--output-delimiter, -z, -F, --complement).
# ---------------------------------------------------------------------------

if(PERL_EXECUTABLE)
    add_test(
        NAME    integ_cut_pl
        COMMAND ${PERL_EXECUTABLE}
                    ${INTEG_DIR}/run_cut_tests.pl
                    $<TARGET_FILE:${CUT_TARGET}>
    )
    add_integration_test(integ_cut_pl)
    set_tests_properties(integ_cut_pl PROPERTIES
        LABELS   "integration;coreutils;cut_pl"
        TIMEOUT  120
    )
else()
    message(WARNING
        "Integration: perl not found — integ_cut_pl will not be registered")
endif()

# ---------------------------------------------------------------------------
# 4.  Coreutils shell test scripts
#     These require the coreutils test infrastructure (tests/init.sh).
#     run_shell_test.sh exits 77 (SKIP) when the infrastructure is absent.
# ---------------------------------------------------------------------------

if(BASH_EXECUTABLE)
    foreach(shell_script
            bounded-memory.sh
            cut-huge-range.sh
            mb-non-utf8.sh)

        string(REPLACE ".sh" "" _tname "${shell_script}")
        set(_tname "integ_${_tname}")

        add_test(
            NAME    ${_tname}
            COMMAND ${BASH_EXECUTABLE}
                        ${INTEG_DIR}/run_shell_test.sh
                        ${INTEG_DIR}/${shell_script}
                        $<TARGET_FILE:${CUT_TARGET}>
        )
        add_integration_test(${_tname})
        set_tests_properties(${_tname} PROPERTIES
            LABELS           "integration;coreutils;shell"
            SKIP_RETURN_CODE 77
        )
    endforeach()
else()
    message(WARNING
        "Integration: bash not found — coreutils shell tests will not be registered")
endif()
