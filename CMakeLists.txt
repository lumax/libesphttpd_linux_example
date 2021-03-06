cmake_minimum_required(VERSION 3.9)

enable_testing()

include(ExternalProject)

set(DEFAULT_INSTALL_DIR ${CMAKE_BINARY_DIR}/install)

set(ARGS "-DCMAKE_INSTALL_PREFIX=${DEFAULT_INSTALL_DIR}")

message("${CMAKE_BUILD_TYPE}")

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
	message("appending CMAKE_BUILD_TYPE")
	list(APPEND ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif(NOT ${CMAKE_BUILD_TYPE} STREQUAL "")

message("ARGS is ${ARGS}")

ExternalProject_Add(esphttpd
	SOURCE_DIR "${CMAKE_SOURCE_DIR}/libesphttpd"
	DOWNLOAD_COMMAND ""
	UPDATE_COMMAND ""
	CMAKE_ARGS "${ARGS}"
)

ExternalProject_Add(httpd
	SOURCE_DIR "${CMAKE_SOURCE_DIR}/httpd"
	DOWNLOAD_COMMAND ""
	UPDATE_COMMAND ""
	CMAKE_ARGS "${ARGS}"
)

ExternalProject_Add(tests
	SOURCE_DIR "${CMAKE_SOURCE_DIR}/tests"
	DOWNLOAD_COMMAND ""
	UPDATE_COMMAND ""
	CMAKE_ARGS "${ARGS}"
)

add_test(NAME test_post
	COMMAND ${DEFAULT_INSTALL_DIR}/bin/test_post.sh
	WORKING_DIRECTORY ${DEFAULT_INSTALL_DIR}/bin
)

add_test(NAME test_too_long_url
	COMMAND ${DEFAULT_INSTALL_DIR}/bin/test_too_long_url.sh
	WORKING_DIRECTORY ${DEFAULT_INSTALL_DIR}/bin
)
