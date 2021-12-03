# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

# Normally this would be handled by find_package(cppgraphqlgen CONFIG).
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/cppgraphqlgen-functions.cmake)

if(GRAPHQL_UPDATE_SAMPLES)
  update_graphql_schema_files(separate_nointrospection ../schema.today.graphql Today today --stubs --no-introspection)
endif()

add_graphql_schema_no_introspection_target(separate_nointrospection)
