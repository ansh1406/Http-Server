cmake_minimum_required(VERSION 3.10)
project(HttpServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB SRC_FILES "${SRC_DIR}/*.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(BUILD_LIBRARY_TYPE "STATIC" CACHE STRING "Library type to build: SHARED or STATIC")
set_property(CACHE BUILD_LIBRARY_TYPE PROPERTY STRINGS SHARED STATIC)

add_library(http_objects OBJECT ${SRC_FILES})
target_include_directories(http_objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(http_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(http_objects PRIVATE -O2)

add_library(http ${BUILD_LIBRARY_TYPE} $<TARGET_OBJECTS:http_objects>)

if(BUILD_LIBRARY_TYPE STREQUAL "SHARED")
	set_target_properties(http PROPERTIES
		PREFIX ""
		OUTPUT_NAME "libhttp"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
elseif(BUILD_LIBRARY_TYPE STREQUAL "STATIC")
	set_target_properties(http PROPERTIES
		PREFIX ""
		OUTPUT_NAME "libhttp"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
endif()

set_target_properties(http PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(http PRIVATE -O2)

add_custom_command(TARGET http POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/includes"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/includes/http.hpp"
		"${CMAKE_CURRENT_BINARY_DIR}/includes/http.hpp"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/includes/http_request.hpp"
		"${CMAKE_CURRENT_BINARY_DIR}/includes/http_request.hpp"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/includes/http_response.hpp"
		"${CMAKE_CURRENT_BINARY_DIR}/includes/http_response.hpp"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/includes/http_constants.hpp"
		"${CMAKE_CURRENT_BINARY_DIR}/includes/http_constants.hpp"
)