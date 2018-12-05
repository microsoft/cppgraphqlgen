# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#[=======================================================================[.rst:
cppgraphqlgen
-------------

The following import targets are created

::

  cppgraphqlgen::graphqlservice
  cppgraphqlgen::schemagen
#]=======================================================================]

include(CMakeFindDependencyMacro)
find_package(pegtl REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/cppgraphqlgen-targets.cmake")
