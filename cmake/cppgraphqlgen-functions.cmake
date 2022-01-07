# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

get_filename_component(GRAPHQL_UPDATE_SCHEMA_FILES_SCRIPT
  "${CMAKE_CURRENT_LIST_DIR}/cppgraphqlgen-update-schema-files.cmake"
  ABSOLUTE)

get_filename_component(GRAPHQL_UPDATE_CLIENT_FILES_SCRIPT
  "${CMAKE_CURRENT_LIST_DIR}/cppgraphqlgen-update-client-files.cmake"
  ABSOLUTE)

function(update_graphql_schema_files SCHEMA_TARGET SCHEMA_GRAPHQL SCHEMA_PREFIX SCHEMA_NAMESPACE)
  set_property(DIRECTORY APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${SCHEMA_TARGET}_schema_files)

  # Collect optional arguments
  set(ADDITIONAL_SCHEMAGEN_ARGS "")
  if(ARGC GREATER 4)
    math(EXPR LAST_ARG "${ARGC} - 1")
    foreach(ARGN RANGE 4 ${LAST_ARG})
      set(NEXT_ARG "${ARGV${ARGN}}")
      list(APPEND ADDITIONAL_SCHEMAGEN_ARGS "${NEXT_ARG}")
    endforeach()
  endif()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_TARGET}_schema_files
    COMMAND
      ${CMAKE_COMMAND} "-DSCHEMA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DSCHEMA_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
      "-DSCHEMAGEN_PROGRAM=$<TARGET_FILE:cppgraphqlgen::schemagen>"
      "-DSCHEMA_TARGET=${SCHEMA_TARGET}" "-DSCHEMA_GRAPHQL=${SCHEMA_GRAPHQL}"
      "-DSCHEMA_PREFIX=${SCHEMA_PREFIX}" "-DSCHEMA_NAMESPACE=${SCHEMA_NAMESPACE}"
      "-DADDITIONAL_SCHEMAGEN_ARGS=${ADDITIONAL_SCHEMAGEN_ARGS}"
      -P ${GRAPHQL_UPDATE_SCHEMA_FILES_SCRIPT}
    DEPENDS ${SCHEMA_GRAPHQL} ${GRAPHQL_UPDATE_SCHEMA_FILES_SCRIPT} cppgraphqlgen::schemagen
    COMMENT "Generating ${SCHEMA_TARGET} GraphQL schema"
    VERBATIM)
endfunction()

function(add_graphql_schema_target SCHEMA_TARGET)
  add_custom_target(${SCHEMA_TARGET}_update_schema ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_TARGET}_schema_files)

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_TARGET}_schema_files)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_TARGET}_schema_files SCHEMA_FILES)
    add_library(${SCHEMA_TARGET}_schema STATIC ${SCHEMA_FILES})
    add_dependencies(${SCHEMA_TARGET}_schema ${SCHEMA_TARGET}_update_schema)
    target_include_directories(${SCHEMA_TARGET}_schema PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(${SCHEMA_TARGET}_schema PUBLIC cppgraphqlgen::graphqlservice)
  endif()
endfunction()

function(update_graphql_client_files CLIENT_TARGET SCHEMA_GRAPHQL REQUEST_GRAPHQL CLIENT_PREFIX CLIENT_NAMESPACE)
  set_property(DIRECTORY APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${CLIENT_TARGET}_client_files)

  # Collect optional arguments
  set(ADDITIONAL_CLIENTGEN_ARGS "")
  if(ARGC GREATER 5)
    math(EXPR LAST_ARG "${ARGC} - 1")
    foreach(ARGN RANGE 5 ${LAST_ARG})
      set(NEXT_ARG "${ARGV${ARGN}}")
      list(APPEND ADDITIONAL_CLIENTGEN_ARGS "${NEXT_ARG}")
    endforeach()
  endif()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${CLIENT_TARGET}_client_files
    COMMAND
      ${CMAKE_COMMAND} "-DCLIENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DCLIENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
      "-DCLIENTGEN_PROGRAM=$<TARGET_FILE:cppgraphqlgen::clientgen>" "-DCLIENT_TARGET=${CLIENT_TARGET}"
      "-DSCHEMA_GRAPHQL=${SCHEMA_GRAPHQL}" "-DREQUEST_GRAPHQL=${REQUEST_GRAPHQL}"
      "-DCLIENT_PREFIX=${CLIENT_PREFIX}" "-DCLIENT_NAMESPACE=${CLIENT_NAMESPACE}"
      "-DADDITIONAL_CLIENTGEN_ARGS=${ADDITIONAL_CLIENTGEN_ARGS}"
      -P ${GRAPHQL_UPDATE_CLIENT_FILES_SCRIPT}
    DEPENDS ${SCHEMA_GRAPHQL} ${REQUEST_GRAPHQL} ${GRAPHQL_UPDATE_CLIENT_FILES_SCRIPT} cppgraphqlgen::clientgen
    COMMENT "Generating ${CLIENT_TARGET} client"
    VERBATIM)
endfunction()

function(add_graphql_client_target CLIENT_TARGET)
  add_custom_target(${CLIENT_TARGET}_update_client ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${CLIENT_TARGET}_client_files)

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${CLIENT_TARGET}_client_files)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/${CLIENT_TARGET}_client_files CLIENT_FILES)
    add_library(${CLIENT_TARGET}_client STATIC ${CLIENT_FILES})
    add_dependencies(${CLIENT_TARGET}_client ${CLIENT_TARGET}_update_client)
    target_include_directories(${CLIENT_TARGET}_client PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(${CLIENT_TARGET}_client PUBLIC cppgraphqlgen::graphqlclient)
  endif()
endfunction()
