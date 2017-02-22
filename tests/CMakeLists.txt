add_library(unity unity/src/unity.c)

if(ENABLE_CJSON_TEST)
    #copy test files
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/inputs")
    file(GLOB test_files "inputs/*")
    file(COPY ${test_files} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/inputs/")

    set(unity_tests
        parse_examples
        parse_number
        parse_hex4
        parse_string
        parse_array
        parse_object
        parse_value
    )

    add_library(test-common common.c)

    option(ENABLE_VALGRIND OFF "Enable the valgrind memory checker for the tests.")
    if (ENABLE_VALGRIND)
        find_program(MEMORYCHECK_COMMAND valgrind)
        if ("${MEMORYCHECK_COMMAND}" MATCHES "MEMORYCHECK_COMMAND-NOTFOUND")
            message(WARNING "Valgrind couldn't be found.")
            unset(MEMORYCHECK_COMMAND)
        else()
            set(MEMORYCHECK_COMMAND_OPTIONS --trace-children=yes --leak-check=full --error-exitcode=1)
        endif()
    endif()

    foreach(unity_test ${unity_tests})
        add_executable("${unity_test}" "${unity_test}.c")
        target_link_libraries("${unity_test}" "${CJSON_LIB}" unity test-common)
        if(MEMORYCHECK_COMMAND)
            add_test(NAME "${unity_test}"
                COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "./${unity_test}")
        else()
            add_test(NAME "${unity_test}"
                COMMAND "./${unity_test}")
        endif()
    endforeach()
endif()
