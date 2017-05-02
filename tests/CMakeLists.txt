if(ENABLE_CJSON_TEST)
    add_library(unity unity/src/unity.c)

    # Disable -Werror for Unity
    if (FLAG_SUPPORTED_Werror)
        if ("${CMAKE_VERSION}" VERSION_LESS "2.8.12")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
        else()
            target_compile_options(unity PRIVATE "-Wno-error")
        endif()
    endif()
    # Disable -fvisibility=hidden for Unity
    if (FLAG_SUPPORTED_fvisibilityhidden)
        if ("${CMAKE_VERSION}" VERSION_LESS "2.8.12")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=default")
        else()
            target_compile_options(unity PRIVATE "-fvisibility=default")
        endif()
    endif()
    # Disable -fsanitize=float-divide-by-zero for Unity (GCC bug on x86 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80097)
    if (FLAG_SUPPORTED_fsanitizefloatdividebyzero AND (CMAKE_C_COMPILER_ID STREQUAL "GNU"))
        if ("${CMAKE_VERSION}" VERSION_LESS "2.8.12")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-sanitize=float-divide-by-zero")
        else()
            target_compile_options(unity PRIVATE "-fno-sanitize=float-divide-by-zero")
        endif()
    endif()

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
        print_string
        print_number
        print_array
        print_object
        print_value
        misc_tests
        parse_with_opts
        compare_tests
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
                COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/${unity_test}")
        else()
            add_test(NAME "${unity_test}"
                COMMAND "./${unity_test}")
        endif()
    endforeach()

    add_dependencies(check ${unity_tests})

    if (ENABLE_CJSON_UTILS)
        #copy test files
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/json-patch-tests")
        file(GLOB test_files "json-patch-tests/*")
        file(COPY ${test_files} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/json-patch-tests/")

        set (cjson_utils_tests
            json_patch_tests
            old_utils_tests)

        foreach (cjson_utils_test ${cjson_utils_tests})
            add_executable("${cjson_utils_test}" "${cjson_utils_test}.c")
            target_link_libraries("${cjson_utils_test}" "${CJSON_LIB}" "${CJSON_UTILS_LIB}" unity test-common)
            if(MEMORYCHECK_COMMAND)
                add_test(NAME "${cjson_utils_test}"
                    COMMAND "${MEMORYCHECK_COMMAND}" ${MEMORYCHECK_COMMAND_OPTIONS} "${CMAKE_CURRENT_BINARY_DIR}/${cjson_utils_test}")
            else()
                add_test(NAME "${cjson_utils_test}"
                    COMMAND "./${cjson_utils_test}")
            endif()
        endforeach()

        add_dependencies(check ${cjson_utils_tests})
    endif()
endif()
