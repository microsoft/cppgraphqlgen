# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

add_subdirectory(client)
add_subdirectory(learn)
add_subdirectory(today)
add_subdirectory(validation)

if(GRAPHQL_BUILD_HTTP_SAMPLE)
  if(EXISTS "${Boost_INCLUDE_DIR}/boost/beast.hpp")
    add_subdirectory(proxy)
  else()
    message(WARNING "GRAPHQL_BUILD_HTTP_SAMPLE requires the Boost.Beast header-only library. Not found in ${Boost_INCLUDE_DIR}.")
  endif()
endif()
