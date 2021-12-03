# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

if(GRAPHQL_BUILD_TESTS)
  add_subdirectory(schema)
  add_library(validationgraphql STATIC
    ValidationMock.cpp)
  target_link_libraries(validationgraphql PUBLIC validation_schema)
  target_include_directories(validationgraphql PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  add_bigobj_flag(validationgraphql)
endif()
