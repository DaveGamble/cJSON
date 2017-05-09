option(ENABLE_FUZZING "Create executables and targets for fuzzing cJSON with afl." Off)
if (ENABLE_FUZZING)
    find_program(AFL_FUZZ afl-fuzz)
    if ("${AFL_FUZZ}" MATCHES "AFL_FUZZ-NOTFOUND")
        message(FATAL_ERROR "Couldn't find afl-fuzz.")
    endif()

    option(ENABLE_LIBFUZZER "Enable fuzzing with libfuzzer (only works with llvm 5 which hasn't been release at this point)" Off)
    if (ENABLE_LIBFUZZER)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=fuzzer")
    endif()

    add_library(fuzz-target fuzz-target.c)
    target_link_libraries(fuzz-target "${CJSON_LIB}")

    add_executable(afl-main afl.c)
    target_link_libraries(afl-main fuzz-target)

    if (NOT ENABLE_SANITIZERS)
        message(FATAL_ERROR "Enable sanitizers with -DENABLE_SANITIZERS=On to do fuzzing.")
    endif()


    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")

    add_custom_target(afl
        COMMAND "${AFL_FUZZ}" -i "${CMAKE_CURRENT_SOURCE_DIR}/inputs" -o "${CMAKE_CURRENT_BINARY_DIR}/findings" -x "${CMAKE_CURRENT_SOURCE_DIR}/json.dict" -- "${CMAKE_CURRENT_BINARY_DIR}/afl-main" "@@"
        DEPENDS afl-main)


endif()
