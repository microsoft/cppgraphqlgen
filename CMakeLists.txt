# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" VERSION_FILE)

# Get the latest release tag from git if possible.
execute_process(COMMAND git describe --tags --long
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE LATEST_VERSION_TAG
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if("${LATEST_VERSION_TAG}" MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)-[0-9]+-[0-9a-z]+$")
  # The latest tag matched the pattern we expected, extract the version number.
  set(LATEST_VERSION "${CMAKE_MATCH_1}")
  message(STATUS "Latest tagged release: v${LATEST_VERSION}")

  # Update version.txt so source archives without the release tags can still get the same version.
  file(WRITE "${VERSION_FILE}" "${LATEST_VERSION}")
else()
  # Just read the last updated version from version.txt.
  file(READ "${VERSION_FILE}" LATEST_VERSION)
endif()

message(STATUS "Latest version: ${LATEST_VERSION}")

project(cppgraphqlgen VERSION ${LATEST_VERSION})

set(CMAKE_CXX_STANDARD 17)

set(GRAPHQL_INSTALL_INCLUDE_DIR include CACHE PATH "Header file install directory")
set(GRAPHQL_INSTALL_TOOLS_DIR bin CACHE PATH "schemagen install directory")
set(GRAPHQL_INSTALL_CMAKE_DIR lib/cmake CACHE PATH "CMake config files install directory")
set(GRAPHQL_INSTALL_CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Configurations which perform a full install")

option(BUILD_SHARED_LIBS "Build shared libraries instead of static libs" OFF)

function(add_bigobj_flag target)
  if(MSVC)
    # MSVC requires the /bigobj flag if the number of sections gets too big.
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()

find_package(Threads MODULE REQUIRED)

find_package(pegtl 3.0.0 QUIET CONFIG)
if(NOT pegtl_FOUND)
  # If a compatible version of PEGTL is not already installed, build and install it from the submodule directory.
  set(PEGTL_BUILD_TESTS OFF CACHE BOOL "Disable PEGTL tests")
  set(PEGTL_BUILD_EXAMPLES OFF CACHE BOOL "Disable PEGTL examples")
  set(PEGTL_INSTALL_INCLUDE_DIR ${GRAPHQL_INSTALL_INCLUDE_DIR} CACHE STRING "Override PEGTL include install directory")
  set(PEGTL_INSTALL_CMAKE_DIR ${GRAPHQL_INSTALL_CMAKE_DIR}/pegtl CACHE STRING "Override PEGTL cmake install directory")
  add_subdirectory(PEGTL)
endif()

option(GRAPHQL_BUILD_SCHEMAGEN "Build the schemagen tool." ON)

if(GRAPHQL_BUILD_SCHEMAGEN)
  option(GRAPHQL_UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests." ON)
else()
  set(GRAPHQL_UPDATE_SAMPLES OFF CACHE BOOL "Disable regenerating samples." FORCE)
endif()

add_subdirectory(cmake)
add_subdirectory(src)

if(GRAPHQL_BUILD_TESTS OR GRAPHQL_UPDATE_SAMPLES)
  add_subdirectory(samples)
  
  if(GRAPHQL_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
  endif()
endif()
