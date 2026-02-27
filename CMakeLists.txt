cmake_minimum_required(VERSION 3.10)
project(HttpServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define source and include directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Gather all source files
file(GLOB SRC_FILES "${SRC_DIR}/*.cpp")

# Define the library type (STATIC or SHARED)
set(BUILD_LIBRARY_TYPE "STATIC" CACHE STRING "Library type to build: SHARED or STATIC")
set_property(CACHE BUILD_LIBRARY_TYPE PROPERTY STRINGS SHARED STATIC)

# Create an object library for the source files
add_library(http_objects OBJECT ${SRC_FILES})
target_include_directories(http_objects
    PUBLIC
    $<BUILD_INTERFACE:${INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(http_objects PUBLIC cxx_std_11)
set_target_properties(http_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Create the static or shared library from the object files
add_library(http ${BUILD_LIBRARY_TYPE} $<TARGET_OBJECTS:http_objects>)
target_include_directories(http
    PUBLIC
    $<BUILD_INTERFACE:${INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(http PUBLIC cxx_std_11)
set_target_properties(http PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Install the library and headers
include(GNUInstallDirs)
install(TARGETS http
    EXPORT http-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT http-targets
    FILE httpTargets.cmake
    NAMESPACE Http::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/http
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/httpConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/httpConfig.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/http
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/httpConfigVersion.cmake"
    VERSION 4.2.0
    COMPATIBILITY SameMinorVersion
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/httpConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/httpConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/http
)