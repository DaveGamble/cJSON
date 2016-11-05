cmake_minimum_required(VERSION 2.8)

set(CJSON_VERSION_MAJOR 1)
set(CJSON_VERSION_MINOR 0)
set(CJSON_VERSION_PATCH 0)
set(CJSON_VERSION_SO 1)
set(CJSON_UTILS_VERSION_SO 1)
set(CJSON_VERSION "${CJSON_VERSION_MAJOR}.${CJSON_VERSION_MINOR}.${CJSON_VERSION_PATCH}")

add_compile_options(-ansi -pedantic -Wall -Wextra -Werror -Wstrict-prototypes -Wwrite-strings)

#variables for pkg-config
set(prefix ${CMAKE_INSTALL_PREFIX})
set(libdir "${CMAKE_INSTALL_PREFIX}/lib")
set(version ${CJSON_VERSION})

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

#cJSON
set(PROJ_CJSON cJSON)
set(CJSON_LIB cjson)
project(${PROJ_CJSON} C)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

add_library(${CJSON_LIB} ${HEADERS} ${SOURCES})
if (NOT WIN32)
    target_link_libraries(${CJSON_LIB} m)
endif()

configure_file("${cJSON_SOURCE_DIR}/libcjson.pc.in"
    "${cJSON_BINARY_DIR}/libcjson.pc" @ONLY)

install(TARGETS ${CJSON_LIB} DESTINATION lib${LIB_SUFFIX})
install(FILES cJSON.h DESTINATION include/cjson)
install (FILES ${cJSON_BINARY_DIR}/libcjson.pc DESTINATION lib/pkgconfig)

set_target_properties(${CJSON_LIB}
    PROPERTIES
        SOVERSION ${CJSON_VERSION_SO}
        VERSION ${CJSON_VERSION})

#cJSON_Utils
set(PROJ_CJSON_UTILS cJSON_utils)
set(CJSON_UTILS_LIB cjson_utils)
project(${PROJ_CJSON_UTILS} C)

file(GLOB HEADERS_UTILS cJSON_Utils.h)
set(SOURCES_UTILS cJSON_Utils.c)

add_library(${CJSON_UTILS_LIB} ${HEADERS_UTILS} ${SOURCES_UTILS})
target_link_libraries(${CJSON_UTILS_LIB} ${CJSON_LIB})

configure_file("${cJSON_utils_SOURCE_DIR}/libcjson_utils.pc.in"
    "${cJSON_utils_BINARY_DIR}/libcjson_utils.pc" @ONLY)

install(TARGETS ${CJSON_UTILS_LIB} DESTINATION lib${LIB_SUFFIX})
install(FILES cJSON_Utils.h DESTINATION include/cjson)
install (FILES ${cJSON_utils_BINARY_DIR}/libcjson.pc DESTINATION lib/pkgconfig)

set_target_properties(${CJSON_UTILS_LIB}
    PROPERTIES
        SOVERSION ${CJSON_UTILS_VERSION_SO}
        VERSION ${CJSON_VERSION})

option(ENABLE_CJSON_TEST "Enable building cJSON test" OFF)
if(ENABLE_CJSON_TEST)
    set(TEST_CJSON cJSON_test)
    add_executable(${TEST_CJSON} test.c)
    target_link_libraries(${TEST_CJSON} ${CJSON_LIB})

    set(TEST_CJSON_UTILS cJSON_test_utils)
    add_executable(${TEST_CJSON_UTILS} test_utils.c)
    target_link_libraries(${TEST_CJSON_UTILS} ${CJSON_UTILS_LIB})
endif()
