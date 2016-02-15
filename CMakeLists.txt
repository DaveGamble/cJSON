cmake_minimum_required(VERSION 2.8)

set(PROJ_CJSON cJSON)

project(${PROJ_CJSON} C)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

add_library(${PROJ_CJSON} STATIC ${HEADERS} ${SOURCES})
if (NOT WIN32)
	target_link_libraries(${PROJ_CJSON} m)
endif()

add_library(${PROJ_CJSON}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJ_CJSON}.shared PROPERTIES OUTPUT_NAME cJSON)
if (NOT WIN32)
	target_link_libraries(${PROJ_CJSON}.shared m)
endif()


set(PROJ_CJSON_UTILS cJSON_utils)

project(${PROJ_CJSON_UTILS} C)

file(GLOB HEADERS_UTILS cJSON_Utils.h)
set(SOURCES_UTILS cJSON_Utils.c)

add_library(${PROJ_CJSON_UTILS} STATIC ${HEADERS_UTILS} ${SOURCES_UTILS})
target_link_libraries(${PROJ_CJSON_UTILS} ${PROJ_CJSON})

add_library(${PROJ_CJSON_UTILS}.shared SHARED ${HEADERS_UTILS} ${SOURCES_UTILS})
set_target_properties(${PROJ_CJSON_UTILS}.shared PROPERTIES OUTPUT_NAME cJSON_utils)
target_link_libraries(${PROJ_CJSON_UTILS}.shared ${PROJ_CJSON}.shared)

install (TARGETS ${PROJ_CJSON} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_CJSON}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES cJSON.h DESTINATION include/cJSON)
install (TARGETS ${PROJ_CJSON_UTILS} DESTINATION lib${LIB_SUFFIX})
install (TARGETS ${PROJ_CJSON_UTILS}.shared DESTINATION lib${LIB_SUFFIX})
install (FILES cJSON_Utils.h DESTINATION include/cJSON)

option(ENABLE_CJSON_TEST "Enable building cJSON test" OFF)
if(ENABLE_CJSON_TEST)
	set(TEST_CJSON cJSON_test)
	add_executable(${TEST_CJSON} test.c)
	target_link_libraries(${TEST_CJSON} ${PROJ_CJSON}.shared)

	set(TEST_CJSON_UTILS cJSON_test_utils)
	add_executable(${TEST_CJSON_UTILS} test_utils.c)
	target_link_libraries(${TEST_CJSON_UTILS} ${PROJ_CJSON_UTILS}.shared)
endif()
