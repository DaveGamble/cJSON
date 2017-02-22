set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8)

subdirs(tests)

include(GNUInstallDirs)

project(cJSON C)

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 0)
set(CJSON_VERSION_SO 1)
set(CJSON_UTILS_VERSION_SO 1)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")


set(custom_compiler_flags)

include(CheckCCompilerFlag)
option(ENABLE_CUSTOM_COMPILER_FLAGS "Enables custom compiler flags for Clang and GCC" ON)
if (ENABLE_CUSTOM_COMPILER_FLAGS)
    list(APPEND custom_compiler_flags
        -std=c89
        -pedantic
        -Wall
        -Wextra
        -Werror
        -Wstrict-prototypes
        -Wwrite-strings
        -Wshadow
        -Winit-self
        -Wcast-align
        -Wformat=2
        -Wmissing-prototypes
        -Wstrict-overflow=2
        -Wcast-qual
        -Wundef
        -Wswitch-default
        -Wconversion
        -Wc++-compat
        -fstack-protector-strong
        )
endif()

option(ENABLE_SANITIZERS "Enables AddressSanitizer and UndefinedBehaviorSanitizer." OFF)
if (ENABLE_SANITIZERS)
    list(APPEND custom_compiler_flags
        -fno-omit-frame-pointer
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
        -fsanitize-address-use-after-scope
        -fsanitize=integer
        -01
        -fno-sanitize-recover
        )
endif()

# apply custom compiler flags
foreach(compiler_flag ${custom_compiler_flags})
    #remove problematic characters
    string(REGEX REPLACE "[^a-zA-Z0-9]" "" current_variable ${compiler_flag})

    CHECK_C_COMPILER_FLAG(${compiler_flag} "FLAG_SUPPORTED_${current_variable}")
    if (FLAG_SUPPORTED_${current_variable})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${compiler_flag}")
    endif()
endforeach()

#variables for pkg-config
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "${CMAKE_INSTALL_LIBDIR}")
set(version "${PROJECT_VERSION}")
set(includedir "${CMAKE_INSTALL_INCLUDEDIR}")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ENABLE_TARGET_EXPORT "Enable exporting of CMake targets. Disable when it causes problems!" ON)

#cJSON
set(CJSON_LIB cjson)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

add_library("${CJSON_LIB}" "${HEADERS}" "${SOURCES}")
if (NOT WIN32)
    target_link_libraries("${CJSON_LIB}" m)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libcjson.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libcjson.pc" @ONLY)

install(FILES cJSON.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cjson")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libcjson.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(TARGETS "${CJSON_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}" EXPORT "${CJSON_LIB}")
if(ENABLE_TARGET_EXPORT)
    # export library information for CMake projects
    install(EXPORT "${CJSON_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cJSON")
endif()

set_target_properties("${CJSON_LIB}"
    PROPERTIES
        SOVERSION "${CJSON_VERSION_SO}"
        VERSION "${PROJECT_VERSION}")

#cJSON_Utils
option(ENABLE_CJSON_UTILS "Enable building the cJSON_Utils library." OFF)
if(ENABLE_CJSON_UTILS)
    set(CJSON_UTILS_LIB cjson_utils)

    file(GLOB HEADERS_UTILS cJSON_Utils.h)
    set(SOURCES_UTILS cJSON_Utils.c)

    add_library("${CJSON_UTILS_LIB}" "${HEADERS_UTILS}" "${SOURCES_UTILS}")
    target_link_libraries("${CJSON_UTILS_LIB}" "${CJSON_LIB}")

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libcjson_utils.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/libcjson_utils.pc" @ONLY)

    install(TARGETS "${CJSON_UTILS_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}" EXPORT "${CJSON_UTILS_LIB}")
    install(FILES cJSON_Utils.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cjson")
    install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libcjson_utils.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    if(ENABLE_TARGET_EXPORT)
      # export library information for CMake projects
      install(EXPORT "${CJSON_UTILS_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cJSON")
    endif()

    set_target_properties("${CJSON_UTILS_LIB}"
        PROPERTIES
            SOVERSION "${CJSON_UTILS_VERSION_SO}"
            VERSION "${PROJECT_VERSION}")
endif()

# create the other package config files
configure_file(
    cJSONConfig.cmake.in
    ${PROJECT_BINARY_DIR}/cJSONConfig.cmake @ONLY)
configure_file(
    cJSONConfigVersion.cmake.in
    ${PROJECT_BINARY_DIR}/cJSONConfigVersion.cmake @ONLY)

# Install package config files
install(FILES ${PROJECT_BINARY_DIR}/cJSONConfig.cmake
    ${PROJECT_BINARY_DIR}/cJSONConfigVersion.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cJSON")

option(ENABLE_CJSON_TEST "Enable building cJSON test" ON)
if(ENABLE_CJSON_TEST)
    enable_testing()

    set(TEST_CJSON cJSON_test)
    add_executable("${TEST_CJSON}" test.c)
    target_link_libraries("${TEST_CJSON}" "${CJSON_LIB}")

    if(ENABLE_CJSON_UTILS)
        set(TEST_CJSON_UTILS cJSON_test_utils)
        add_executable("${TEST_CJSON_UTILS}" test_utils.c)
        target_link_libraries("${TEST_CJSON_UTILS}" "${CJSON_UTILS_LIB}")
    endif()
endif()
