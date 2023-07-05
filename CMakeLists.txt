set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 3.0)

project(cJSON
    VERSION 1.7.16
    LANGUAGES C)

cmake_policy(SET CMP0054 NEW)  # set CMP0054 policy

include(GNUInstallDirs)

set(CJSON_VERSION_SO 1)
set(CJSON_UTILS_VERSION_SO 1)

set(custom_compiler_flags)

include(CheckCCompilerFlag)
option(ENABLE_CUSTOM_COMPILER_FLAGS "Enables custom compiler flags" ON)
if (ENABLE_CUSTOM_COMPILER_FLAGS)
    if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
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
            -Wcomma
            -Wdouble-promotion
            -Wparentheses
            -Wformat-overflow
            -Wunused-macros
            -Wmissing-variable-declarations
            -Wused-but-marked-unused
            -Wswitch-enum
        )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        # Disable warning c4001 - nonstandard extension 'single line comment' was used
        # Define _CRT_SECURE_NO_WARNINGS to disable deprecation warnings for "insecure" C library functions
        list(APPEND custom_compiler_flags
            /GS
            /Za
            /sdl
            /W4
            /wd4001
            /D_CRT_SECURE_NO_WARNINGS
        )
    endif()
endif()

option(ENABLE_SANITIZERS "Enables AddressSanitizer and UndefinedBehaviorSanitizer." OFF)
if (ENABLE_SANITIZERS)
    list(APPEND custom_compiler_flags
        -fno-omit-frame-pointer
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=float-cast-overflow
        -fsanitize-address-use-after-scope
        -fsanitize=integer
        -01
        -fno-sanitize-recover
        )
endif()

option(ENABLE_SAFE_STACK "Enables the SafeStack instrumentation pass by the Code Pointer Integrity Project" OFF)
if (ENABLE_SAFE_STACK)
    if (ENABLE_SANITIZERS)
        message(FATAL_ERROR "ENABLE_SAFE_STACK cannot be used in combination with ENABLE_SANITIZERS")
    endif()
    list(APPEND custom_compiler_flags
        -fsanitize=safe-stack
        )
endif()

option(ENABLE_PUBLIC_SYMBOLS "Export library symbols." On)
if (ENABLE_PUBLIC_SYMBOLS)
    list(APPEND custom_compiler_flags -fvisibility=hidden)
    add_definitions(-DCJSON_EXPORT_SYMBOLS -DCJSON_API_VISIBILITY)
endif()
option(ENABLE_HIDDEN_SYMBOLS "Hide library symbols." Off)
if (ENABLE_HIDDEN_SYMBOLS)
    add_definitions(-DCJSON_HIDE_SYMBOLS -UCJSON_API_VISIBILITY)
endif()

# apply custom compiler flags
foreach(compiler_flag ${custom_compiler_flags})
    #remove problematic characters
    string(REGEX REPLACE "[^a-zA-Z0-9]" "" current_variable ${compiler_flag})

    CHECK_C_COMPILER_FLAG(${compiler_flag} "FLAG_SUPPORTED_${current_variable}")
    if (FLAG_SUPPORTED_${current_variable})
        list(APPEND supported_compiler_flags)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${compiler_flag}")
    endif()
endforeach()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${supported_compiler_flags}")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ENABLE_TARGET_EXPORT "Enable exporting of CMake targets. Disable when it causes problems!" ON)

#cJSON
set(CJSON_LIB cjson)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

option(BUILD_SHARED_AND_STATIC_LIBS "Build both shared and static libraries" Off)
option(CJSON_OVERRIDE_BUILD_SHARED_LIBS "Override BUILD_SHARED_LIBS with CJSON_BUILD_SHARED_LIBS" OFF)
option(CJSON_BUILD_SHARED_LIBS "Overrides BUILD_SHARED_LIBS if CJSON_OVERRIDE_BUILD_SHARED_LIBS is enabled" ON)
option(ENABLE_CJSON_VERSION_SO "Enables cJSON so version" ON)

if ((CJSON_OVERRIDE_BUILD_SHARED_LIBS AND CJSON_BUILD_SHARED_LIBS) OR ((NOT CJSON_OVERRIDE_BUILD_SHARED_LIBS) AND BUILD_SHARED_LIBS))
    set(CJSON_LIBRARY_TYPE SHARED)
else()
    set(CJSON_LIBRARY_TYPE STATIC)
endif()


if (NOT BUILD_SHARED_AND_STATIC_LIBS)
    add_library("${CJSON_LIB}" "${CJSON_LIBRARY_TYPE}" "${HEADERS}" "${SOURCES}")
else()
    # See https://cmake.org/Wiki/CMake_FAQ#How_do_I_make_my_shared_and_static_libraries_have_the_same_root_name.2C_but_different_suffixes.3F
    add_library("${CJSON_LIB}" SHARED "${HEADERS}" "${SOURCES}")
    add_library("${CJSON_LIB}-static" STATIC "${HEADERS}" "${SOURCES}")
    set_target_properties("${CJSON_LIB}-static" PROPERTIES OUTPUT_NAME "${CJSON_LIB}")
    set_target_properties("${CJSON_LIB}-static" PROPERTIES PREFIX "lib")
endif()
if (NOT WIN32)
    target_link_libraries("${CJSON_LIB}" m)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/library_config/libcjson.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libcjson.pc" @ONLY)

install(FILES cJSON.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/cjson")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libcjson.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
install(TARGETS "${CJSON_LIB}"
    EXPORT "${CJSON_LIB}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
)
if (BUILD_SHARED_AND_STATIC_LIBS)
    install(TARGETS "${CJSON_LIB}-static"
    EXPORT "${CJSON_LIB}" 
    ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
)
endif()
if(ENABLE_TARGET_EXPORT)
    # export library information for CMake projects
    install(EXPORT "${CJSON_LIB}" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/cJSON")
endif()

if(ENABLE_CJSON_VERSION_SO)
    set_target_properties("${CJSON_LIB}"
        PROPERTIES
            SOVERSION "${CJSON_VERSION_SO}"
            VERSION "${PROJECT_VERSION}")
endif()

#cJSON_Utils
option(ENABLE_CJSON_UTILS "Enable building the cJSON_Utils library." OFF)
if(ENABLE_CJSON_UTILS)
    set(CJSON_UTILS_LIB cjson_utils)

    file(GLOB HEADERS_UTILS cJSON_Utils.h)
    set(SOURCES_UTILS cJSON_Utils.c)

    if (NOT BUILD_SHARED_AND_STATIC_LIBS)
        add_library("${CJSON_UTILS_LIB}" "${CJSON_LIBRARY_TYPE}" "${HEADERS_UTILS}" "${SOURCES_UTILS}")
        target_link_libraries("${CJSON_UTILS_LIB}" "${CJSON_LIB}")
    else()
        add_library("${CJSON_UTILS_LIB}" SHARED "${HEADERS_UTILS}" "${SOURCES_UTILS}")
        target_link_libraries("${CJSON_UTILS_LIB}" "${CJSON_LIB}")
        add_library("${CJSON_UTILS_LIB}-static" STATIC "${HEADERS_UTILS}" "${SOURCES_UTILS}")
        target_link_libraries("${CJSON_UTILS_LIB}-static" "${CJSON_LIB}-static")
        set_target_properties("${CJSON_UTILS_LIB}-static" PROPERTIES OUTPUT_NAME "${CJSON_UTILS_LIB}")
        set_target_properties("${CJSON_UTILS_LIB}-static" PROPERTIES PREFIX "lib")
    endif()

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/library_config/libcjson_utils.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/libcjson_utils.pc" @ONLY)

    install(TARGETS "${CJSON_UTILS_LIB}"
        EXPORT "${CJSON_UTILS_LIB}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
    )
    if (BUILD_SHARED_AND_STATIC_LIBS)
        install(TARGETS "${CJSON_UTILS_LIB}-static" 
        EXPORT "${CJSON_UTILS_LIB}" 
        ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
        )
    endif()
    install(FILES cJSON_Utils.h DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/cjson")
    install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libcjson_utils.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
    if(ENABLE_TARGET_EXPORT)
      # export library information for CMake projects
      install(EXPORT "${CJSON_UTILS_LIB}" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/cJSON")
    endif()

    if(ENABLE_CJSON_VERSION_SO)
        set_target_properties("${CJSON_UTILS_LIB}"
            PROPERTIES
                SOVERSION "${CJSON_UTILS_VERSION_SO}"
                VERSION "${PROJECT_VERSION}")
    endif()
endif()

# create the other package config files
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/library_config/cJSONConfig.cmake.in"
    ${PROJECT_BINARY_DIR}/cJSONConfig.cmake @ONLY)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/library_config/cJSONConfigVersion.cmake.in"
    ${PROJECT_BINARY_DIR}/cJSONConfigVersion.cmake @ONLY)

if(ENABLE_TARGET_EXPORT)
    # Install package config files
    install(FILES ${PROJECT_BINARY_DIR}/cJSONConfig.cmake
        ${PROJECT_BINARY_DIR}/cJSONConfigVersion.cmake
        DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/cJSON")
endif()

option(ENABLE_CJSON_TEST "Enable building cJSON test" ON)
if(ENABLE_CJSON_TEST)
    enable_testing()

    set(TEST_CJSON cJSON_test)
    add_executable("${TEST_CJSON}" test.c)
    target_link_libraries("${TEST_CJSON}" "${CJSON_LIB}")

    add_test(NAME ${TEST_CJSON} COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${TEST_CJSON}")

    # Disable -fsanitize=float-divide-by-zero for cJSON_test
    if (FLAG_SUPPORTED_fsanitizefloatdividebyzero)
        if ("${CMAKE_VERSION}" VERSION_LESS "2.8.12")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-sanitize=float-divide-by-zero")
        else()
            target_compile_options(${TEST_CJSON} PRIVATE "-fno-sanitize=float-divide-by-zero")
        endif()
    endif()

    #"check" target that automatically builds everything and runs the tests
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${TEST_CJSON})
endif()

#Create the uninstall target
option(ENABLE_CJSON_UNINSTALL "Enable creating uninstall target" ON)
if(ENABLE_CJSON_UNINSTALL)
  add_custom_target(uninstall "${CMAKE_COMMAND}" -P
    "${PROJECT_SOURCE_DIR}/library_config/uninstall.cmake")
endif()

# Enable the use of locales
option(ENABLE_LOCALES "Enable the use of locales" ON)
if(ENABLE_LOCALES)
	add_definitions(-DENABLE_LOCALES)
endif()

add_subdirectory(tests)
add_subdirectory(fuzzing)
