# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.28)

if(POLICY CMP0167)
  # Prefer the upstream BoostConfig.cmake file instead of the builtin FindBoost module: https://cmake.org/cmake/help/latest/policy/CMP0167.html
  cmake_policy(SET CMP0167 NEW)
endif()

# Export compile commands for other tools, e.g. SonarLint.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to the last updated version from version.txt.
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.txt" VERSION_FILE)
file(READ "${VERSION_FILE}" LATEST_VERSION)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  # Get the latest release tag from git if possible.
  execute_process(COMMAND git describe --tags --long
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE LATEST_VERSION_TAG
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(LATEST_VERSION_TAG MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)-[0-9]+-[0-9a-z]+$"
    AND CMAKE_MATCH_1 VERSION_GREATER LATEST_VERSION)
    # The latest tag matched the pattern we expected and it's newer, so use that as the latest version.
    set(LATEST_VERSION "${CMAKE_MATCH_1}")

    # Update version.txt so source archives without the release tags still get the same version.
    file(WRITE "${VERSION_FILE}" "${LATEST_VERSION}")
  endif()
endif()

option(GRAPHQL_BUILD_SCHEMAGEN "Build the schemagen tool." ON)
option(GRAPHQL_BUILD_CLIENTGEN "Build the clientgen tool." ON)
option(GRAPHQL_BUILD_TESTS "Build the tests and sample schema library." ON)
option(GRAPHQL_USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)

if(GRAPHQL_BUILD_SCHEMAGEN)
  list(APPEND VCPKG_MANIFEST_FEATURES "schemagen")
endif()

if(GRAPHQL_BUILD_CLIENTGEN)
  list(APPEND VCPKG_MANIFEST_FEATURES "clientgen")
endif()

if(GRAPHQL_BUILD_TESTS)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

if(GRAPHQL_USE_RAPIDJSON)
  list(APPEND VCPKG_MANIFEST_FEATURES "rapidjson")
endif()

if(GRAPHQL_BUILD_SCHEMAGEN AND GRAPHQL_BUILD_CLIENTGEN)
  option(GRAPHQL_UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests." ON)

  if(GRAPHQL_UPDATE_SAMPLES)
    list(APPEND VCPKG_MANIFEST_FEATURES "update-samples")

    option(GRAPHQL_BUILD_HTTP_SAMPLE "Build the HTTP client sample using C++20 coroutines with Boost.Beast/Boost.Asio." ON)

    if(GRAPHQL_BUILD_HTTP_SAMPLE)
      list(APPEND VCPKG_MANIFEST_FEATURES "http-sample")
    endif()
  endif()
else()
  set(GRAPHQL_UPDATE_SAMPLES OFF CACHE BOOL "Disable regenerating samples." FORCE)
  set(GRAPHQL_BUILD_HTTP_SAMPLE OFF CACHE BOOL "Disable regenerating samples." FORCE)
endif()

project(cppgraphqlgen VERSION ${LATEST_VERSION})

set(GRAPHQL_INSTALL_INCLUDE_DIR include CACHE PATH "Header file install directory")
set(GRAPHQL_INSTALL_TOOLS_DIR bin CACHE PATH "schemagen install directory")
set(GRAPHQL_INSTALL_CMAKE_DIR lib/cmake CACHE PATH "CMake config files install directory")
set(GRAPHQL_INSTALL_CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Configurations which perform a full install")

option(BUILD_SHARED_LIBS "Build shared libraries instead of static libs" OFF)

if(VCPKG_TARGET_TRIPLET)
  message(STATUS "Using ${VCPKG_TARGET_TRIPLET} triplet with vcpkg")

  if(MSVC)
    # Match the MSVC runtime if we're using VCPKG with static libraries.
    if(VCPKG_TARGET_TRIPLET MATCHES [[^.+-static$]])
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      message(STATUS "Using MSVC runtime static library")
    else()
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
      message(STATUS "Using MSVC runtime DLL")
    endif()
  endif()
endif()

# Enable more warnings, and treat warnings as errors.
if(MSVC)
  add_compile_options(/W4 /WX /permissive-)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

function(add_bigobj_flag target)
  if(MSVC)
    # MSVC requires the /bigobj flag if the number of sections gets too big.
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()

find_package(Threads MODULE REQUIRED)

find_package(pegtl 3.2.7 QUIET CONFIG)
if(NOT pegtl_FOUND)
  # If a compatible version of PEGTL is not already installed, build and install it from the submodule directory.
  set(PEGTL_BUILD_TESTS OFF CACHE BOOL "Disable PEGTL tests")
  set(PEGTL_BUILD_EXAMPLES OFF CACHE BOOL "Disable PEGTL examples")
  set(PEGTL_INSTALL_INCLUDE_DIR ${GRAPHQL_INSTALL_INCLUDE_DIR} CACHE STRING "Override PEGTL include install directory")
  set(PEGTL_INSTALL_CMAKE_DIR ${GRAPHQL_INSTALL_CMAKE_DIR}/pegtl CACHE STRING "Override PEGTL cmake install directory")
  add_subdirectory(PEGTL)
endif()

option(GRAPHQL_UPDATE_VERSION "Regenerate graphqlservice/internal/Version.h and all of the version info rc files for Windows." ON)

add_subdirectory(cmake)
add_subdirectory(src)

if(GRAPHQL_BUILD_TESTS OR GRAPHQL_UPDATE_SAMPLES)
  add_subdirectory(samples)

  if(GRAPHQL_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
  endif()
endif()
