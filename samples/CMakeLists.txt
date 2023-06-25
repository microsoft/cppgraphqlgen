# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

add_subdirectory(client)
add_subdirectory(learn)
add_subdirectory(today)
add_subdirectory(validation)

if(GRAPHQL_BUILD_HTTP_SAMPLE)
  find_package(Boost QUIET)
  if(Boost_FOUND)
    if(Boost_VERSION VERSION_GREATER_EQUAL "1.81.0")
      try_compile(TEST_RESULT
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/test_boost_beast.cpp
        CMAKE_FLAGS -DINCLUDE_DIRECTORIES:STRING=${Boost_INCLUDE_DIR}
        CXX_STANDARD 20)

      if(TEST_RESULT)
        message(STATUS "Using Boost.Beast ${Boost_VERSION}")
        add_subdirectory(proxy)
      else()
        message(WARNING "GRAPHQL_BUILD_HTTP_SAMPLE requires the Boost.Beast header-only library and a toolchain that supports co_await in Boost.Asio.")
      endif()
    else()
      message(WARNING "GRAPHQL_BUILD_HTTP_SAMPLE requires the Boost.Beast header-only library >= 1.81.0, but only ${Boost_VERSION} was found in ${Boost_INCLUDE_DIR}.")
    endif()
  endif()
endif()
