# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# Normally this would be handled by find_package(cppgraphqlgen CONFIG).
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/cppgraphqlgen-functions.cmake)

if(GRAPHQL_UPDATE_SAMPLES)
  update_graphql_schema_files(scalar ../schema.scalar.graphql Scalar scalar --stubs)
endif()

add_graphql_schema_target(scalar)

if(TARGET scalar_schema)
  # The generated schema header #includes "BigIntScalar.h" (via the @cppType directive) and the
  # generated resolvers reference the service::Argument/service::Result specializations defined in
  # BigIntScalar.cpp, so compile that translation unit into the schema library itself. This keeps
  # the custom scalar's (de)serialization together with the generated code that depends on it and
  # avoids a circular dependency between two static libraries.
  target_sources(scalar_schema PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../BigIntScalar.cpp)
  target_include_directories(scalar_schema PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()
