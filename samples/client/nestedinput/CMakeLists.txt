# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# Normally this would be handled by find_package(cppgraphqlgen CONFIG).
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/cppgraphqlgen-functions.cmake)

if(GRAPHQL_UPDATE_SAMPLES AND GRAPHQL_BUILD_CLIENTGEN)
  update_graphql_client_files(nestedinput schema.graphql query.graphql NestedInput nestedinput)
endif()

add_graphql_client_target(nestedinput)
