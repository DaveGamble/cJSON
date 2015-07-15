cmake_minimum_required(VERSION 2.8)

set(PROJECT cJSON)

project(${PROJECT} C)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

add_library(${PROJECT} STATIC ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT} m)

add_library(${PROJECT}.shared SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT}.shared PROPERTIES OUTPUT_NAME cJSON)
target_link_libraries(${PROJECT}.shared m)


option(ENABLE_CJSON_TEST "Enable building cJSON test" OFF)
if(ENABLE_CJSON_TEST)
	set(TEST_NAME cJSON_test)
	add_executable(${TEST_NAME} test.c)
	target_link_libraries(${TEST_NAME} ${PROJECT})
endif()
