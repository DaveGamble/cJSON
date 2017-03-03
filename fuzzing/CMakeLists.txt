option(ENABLE_FUZZING "Create executables and targets for fuzzing cJSON with afl." Off)
if (ENABLE_FUZZING)
    find_program(AFL_FUZZ afl-fuzz)
    if ("${AFL_FUZZ}" MATCHES "AFL_FUZZ-NOTFOUND")
        message(FATAL_ERROR "Couldn't find afl-fuzz.")
    endif()

    add_executable(afl-main afl.c)
    target_link_libraries(afl-main "${CJSON_LIB}")

    if (NOT ENABLE_SANITIZERS)
        message(FATAL_ERROR "Enable sanitizers with -DENABLE_SANITIZERS=On to do fuzzing.")
    endif()

    option(ENABLE_FUZZING_PRINT "Fuzz printing functions together with parser." On)
    set(fuzz_print_parameter "no")
    if (ENABLE_FUZZING_PRINT)
        set(fuzz_print_parameter "yes")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")

    add_custom_target(afl
        COMMAND "${AFL_FUZZ}" -i "${CMAKE_CURRENT_SOURCE_DIR}/inputs" -o "${CMAKE_CURRENT_BINARY_DIR}/findings" -x "${CMAKE_CURRENT_SOURCE_DIR}/json.dict" -- "${CMAKE_CURRENT_BINARY_DIR}/afl-main" "@@" "${fuzz_print_parameter}"
        DEPENDS afl-main)


endif()
