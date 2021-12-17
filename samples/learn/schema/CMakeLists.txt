# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# Normally this would be handled by find_package(cppgraphqlgen CONFIG).
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/cppgraphqlgen-functions.cmake)

if(GRAPHQL_UPDATE_SAMPLES)
  update_graphql_schema_files(learn schema.learn.graphql StarWars learn)
endif()

add_graphql_schema_target(learn)
