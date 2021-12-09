# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# Normally this would be handled by find_package(cppgraphqlgen CONFIG).
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cppgraphqlgen-functions.cmake)

if(GRAPHQL_UPDATE_SAMPLES)
  update_graphql_schema_files(introspection schema.introspection.graphql Introspection introspection --introspection)

  add_custom_command(
    OUTPUT updated_introspection_schema_headers
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/../../include/graphqlservice/introspection/
    COMMAND ${CMAKE_COMMAND} -E touch updated_introspection_schema_headers
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/introspection_schema_files
    COMMENT "Updating IntrospectionSchema headers")

  add_custom_target(update_introspection_headers ALL
    DEPENDS updated_introspection_schema_headers)
endif()

set_property(DIRECTORY APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS introspection_schema_files)
file(GLOB SCHEMA_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../include/graphqlservice/introspection/*.h)
install(FILES ${SCHEMA_FILES}
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice/introspection)
