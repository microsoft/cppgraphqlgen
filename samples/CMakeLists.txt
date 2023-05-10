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
      if(EXISTS "${Boost_INCLUDE_DIR}/boost/beast.hpp")
        message(STATUS "Using Boost.Beast ${Boost_VERSION}")
        add_subdirectory(proxy)
      else()
        message(WARNING "GRAPHQL_BUILD_HTTP_SAMPLE requires the Boost.Beast header-only library, but it was not found in ${Boost_INCLUDE_DIR}.")
      endif()
    else()
      message(WARNING "GRAPHQL_BUILD_HTTP_SAMPLE requires the Boost.Beast header-only library >= 1.81.0, but only ${Boost_VERSION} was found in ${Boost_INCLUDE_DIR}.")
    endif()
  endif()
endif()
