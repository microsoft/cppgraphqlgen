# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.28)

if(GRAPHQL_BUILD_MODULES)
  add_subdirectory(today)
endif()

add_subdirectory(client)
add_subdirectory(learn)
add_subdirectory(validation)

if(GRAPHQL_BUILD_HTTP_SAMPLE)
  find_package(boost_beast CONFIG QUIET)
  if(boost_beast_FOUND)
    if(boost_beast_VERSION VERSION_GREATER_EQUAL "1.81.0")
      message(STATUS "Using Boost.Beast ${boost_beast_VERSION}")
      add_subdirectory(proxy)
    else()
      message(WARNING "GRAPHQL_BUILD_HTTP_SAMPLE requires the Boost.Beast header-only library >= 1.81.0, but only ${boost_beast_VERSION} was found.")
    endif()
  endif()
endif()
