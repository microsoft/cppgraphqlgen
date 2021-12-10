# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# Normally this would be handled by find_package(cppgraphqlgen CONFIG).
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cppgraphqlgen-functions.cmake)

if(GRAPHQL_UPDATE_SAMPLES)
  update_graphql_schema_files(introspection schema.introspection.graphql Introspection introspection --introspection)

  file(GLOB PRIVATE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
  add_custom_command(
    OUTPUT copied_introspection_schema_headers
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PRIVATE_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/../../include/graphqlservice/introspection/
    COMMAND ${CMAKE_COMMAND} -E touch copied_introspection_schema_headers
    DEPENDS ${PRIVATE_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/introspection_schema_files
    COMMENT "Updating IntrospectionSchema headers")

  add_custom_target(copy_introspection_schema_headers ALL
    DEPENDS copied_introspection_schema_headers)
endif()

file(GLOB PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/graphqlservice/introspection/*.h)
install(FILES ${PUBLIC_HEADERS}
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice/introspection)
