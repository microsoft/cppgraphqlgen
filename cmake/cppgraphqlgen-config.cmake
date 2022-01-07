# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#[=======================================================================[.rst:
cppgraphqlgen
-------------

The following import targets are created:

::

  cppgraphqlgen::graphqlpeg
  cppgraphqlgen::graphqlresponse
  cppgraphqlgen::graphqlservice
  cppgraphqlgen::graphqljson
  cppgraphqlgen::graphqlclient
  cppgraphqlgen::schemagen
  cppgraphqlgen::clientgen

The following functions are defined to help with code generation and build targets:

::

  update_graphql_schema_files
  add_graphql_schema_target
  update_graphql_client_files
  add_graphql_client_target
#]=======================================================================]

include(CMakeFindDependencyMacro)
find_package(Threads REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/cppgraphqlgen-targets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cppgraphqlgen-functions.cmake")
