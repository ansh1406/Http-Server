cmake_minimum_required(VERSION 3.10)
project(HttpServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB SRC_FILES "${SRC_DIR}/*.cpp")

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/includes)

add_library(http SHARED ${SRC_FILES})

set_target_properties(http PROPERTIES
	PREFIX ""
	OUTPUT_NAME "libhttp"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set_target_properties(http PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(http PRIVATE -O2)

add_custom_command(TARGET http POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/includes"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/includes/http.hpp"
		"${CMAKE_BINARY_DIR}/includes/http.hpp"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/includes/http_request.hpp"
		"${CMAKE_BINARY_DIR}/includes/http_request.hpp"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/includes/http_response.hpp"
		"${CMAKE_BINARY_DIR}/includes/http_response.hpp"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/includes/http_constants.hpp"
		"${CMAKE_BINARY_DIR}/includes/http_constants.hpp"
)

message(STATUS "Configured project to build shared library libhttp.so in the build directory")
