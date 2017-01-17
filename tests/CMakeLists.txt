add_library(unity unity/src/unity.c)

if(ENABLE_CJSON_TEST)
    #copy test files
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/inputs")
    file(GLOB test_files "inputs/*")
    file(COPY ${test_files} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/inputs/")

    set(unity_tests
        parse_examples
        parse_number
    )

    foreach(unity_test ${unity_tests})
        add_executable("${unity_test}" "${unity_test}.c")
        target_link_libraries("${unity_test}" "${CJSON_LIB}" unity)
        add_test("${unity_test}" "${unity_test}")
    endforeach()
endif()
