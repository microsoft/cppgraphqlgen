# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.28)

if(GRAPHQL_UPDATE_VERSION)
  # internal/Version.h
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    @ONLY)

  add_custom_command(OUTPUT updated_version_h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different Version.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal
    COMMAND ${CMAKE_COMMAND} -E touch updated_version_h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)

  add_custom_target(update_version_h ALL
    DEPENDS updated_version_h)

  # res/target_version.rc
  set(GRAPHQL_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
  set(GRAPHQL_RC_VERSION_STR "${PROJECT_VERSION}")

  function(update_version_rc target)
    add_dependencies(${target} update_version_h)

    set(GRAPHQL_RC_FILENAME "${target}")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Library.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc
      @ONLY)

    add_custom_command(OUTPUT updated_${target}_version_rc
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${target}_version.rc ${CMAKE_CURRENT_SOURCE_DIR}/../res
      COMMAND ${CMAKE_COMMAND} -E touch updated_${target}_version_rc
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc)

    add_custom_target(update_${target}_version_rc ALL
      DEPENDS updated_${target}_version_rc)
    add_dependencies(${target} update_${target}_version_rc)
  endfunction()
endif()

if(WIN32)
  # Always build the version.rc files with these flags, even if we aren't regenerating them.
  set(CMAKE_RC_FLAGS "/nologo")
  set(CMAKE_RC_FLAGS_DEBUG "/dDEBUG")

  if(BUILD_SHARED_LIBS)
    function(add_version_rc target)
      add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc.res
        COMMAND ${CMAKE_RC_COMPILER} ${CMAKE_RC_FLAGS} $<$<CONFIG:Debug>:${CMAKE_RC_FLAGS_DEBUG}>
          "/fo${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc.res"
          "${CMAKE_CURRENT_SOURCE_DIR}/../res/${target}_version.rc"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../res/${target}_version.rc")
      add_custom_target(build_${target}_version_rc ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc.res)

      add_library(${target}_version OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc.res)
      set_target_properties(${target}_version PROPERTIES LINKER_LANGUAGE CXX)
      add_dependencies(${target}_version build_${target}_version_rc)
      target_link_libraries(${target} PRIVATE ${target}_version)
    endfunction()
  endif()
endif()

function(add_bigobj_flag target)
  if(MSVC)
    # MSVC requires the /bigobj flag if the number of sections gets too big.
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()

file(REAL_PATH ../include/ INCLUDE_ROOT)

# graphql_internal_modules
add_library(graphql_internal_modules OBJECT)
add_library(cppgraphqlgen::graphql_internal_modules ALIAS graphql_internal_modules)
set_target_properties(graphql_internal_modules PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(graphql_internal_modules PUBLIC cxx_std_20)
target_link_libraries(graphql_internal_modules PUBLIC taocpp::pegtl)
target_include_directories(graphql_internal_modules PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
target_sources(graphql_internal_modules PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${INCLUDE_ROOT}
  FILES
    ${INCLUDE_ROOT}/graphqlservice/internal/Awaitable.h
    ${INCLUDE_ROOT}/graphqlservice/internal/Base64.h
    ${INCLUDE_ROOT}/graphqlservice/internal/Grammar.h
    ${INCLUDE_ROOT}/graphqlservice/internal/Introspection.h
    ${INCLUDE_ROOT}/graphqlservice/internal/Schema.h
    ${INCLUDE_ROOT}/graphqlservice/internal/SortedMap.h
    ${INCLUDE_ROOT}/graphqlservice/internal/SyntaxTree.h
    ${INCLUDE_ROOT}/graphqlservice/internal/Version.h)
if(GRAPHQL_BUILD_MODULES)
  target_sources(graphql_internal_modules PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS
      ${INCLUDE_ROOT}
    FILES
      ${INCLUDE_ROOT}/graphqlservice/internal/Awaitable.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/Base64.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/Grammar.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/Introspection.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/Schema.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/SortedMap.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/SyntaxTree.ixx
      ${INCLUDE_ROOT}/graphqlservice/internal/Version.ixx)
endif()

# graphql_introspection_modules
add_library(graphql_introspection_modules OBJECT)
add_library(cppgraphqlgen::graphql_introspection_modules ALIAS graphql_introspection_modules)
set_target_properties(graphql_introspection_modules PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(graphql_introspection_modules PUBLIC cxx_std_20)
target_link_libraries(graphql_introspection_modules PUBLIC taocpp::pegtl)
target_include_directories(graphql_introspection_modules PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
file(GLOB INTROSPECTION_HEADERS ${INCLUDE_ROOT}/graphqlservice/introspection/*.h)
target_sources(graphql_introspection_modules PUBLIC FILE_SET HEADERS
  BASE_DIRS ${INCLUDE_ROOT}
  FILES ${INTROSPECTION_HEADERS})
if(GRAPHQL_BUILD_MODULES)
  file(GLOB INTROSPECTION_MODULES ${INCLUDE_ROOT}/graphqlservice/introspection/*.ixx)
  target_sources(graphql_introspection_modules PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${INCLUDE_ROOT}
    FILES ${INTROSPECTION_MODULES})
endif()

# graphqlpeg
add_library(graphqlpeg SyntaxTree.cpp)
add_library(cppgraphqlgen::graphqlpeg ALIAS graphqlpeg)
target_compile_features(graphqlpeg PUBLIC cxx_std_20)
target_link_libraries(graphqlpeg PUBLIC taocpp::pegtl)
target_sources(graphqlpeg PUBLIC FILE_SET HEADERS
  BASE_DIRS ${INCLUDE_ROOT}
  FILES ${INCLUDE_ROOT}/graphqlservice/GraphQLParse.h)
if(GRAPHQL_BUILD_MODULES)
  target_sources(graphqlpeg PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${INCLUDE_ROOT}
    FILES ${INCLUDE_ROOT}/graphqlservice/Parse.ixx)
endif()
target_include_directories(graphqlpeg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
add_bigobj_flag(graphqlpeg)

if(GRAPHQL_UPDATE_VERSION)
  update_version_rc(graphqlpeg)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlpeg
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLPEG_DLL)

  add_version_rc(graphqlpeg)
endif()

# graphqlresponse
add_library(graphqlresponse
  Base64.cpp
  GraphQLResponse.cpp)
add_library(cppgraphqlgen::graphqlresponse ALIAS graphqlresponse)
target_compile_features(graphqlresponse PUBLIC cxx_std_20)
target_include_directories(graphqlresponse PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
target_link_libraries(graphqlresponse PUBLIC graphql_internal_modules)
target_sources(graphqlresponse PUBLIC FILE_SET HEADERS
  BASE_DIRS ${INCLUDE_ROOT}
  FILES ${INCLUDE_ROOT}/graphqlservice/GraphQLResponse.h)
if(GRAPHQL_BUILD_MODULES)
  target_sources(graphqlresponse PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${INCLUDE_ROOT}
    FILES ${INCLUDE_ROOT}/graphqlservice/Response.ixx)
endif()

if(GRAPHQL_UPDATE_VERSION)
  update_version_rc(graphqlresponse)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlresponse
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLRESPONSE_DLL)

  add_version_rc(graphqlresponse)
endif()

# Common schemagen and clientgen dependencies
if(GRAPHQL_BUILD_SCHEMAGEN OR GRAPHQL_BUILD_CLIENTGEN)
  add_library(generator_util STATIC
    SchemaLoader.cpp
    GeneratorLoader.cpp
    GeneratorUtil.cpp)
  target_compile_features(generator_util PUBLIC cxx_std_20)
  target_link_libraries(generator_util PUBLIC
    graphqlpeg
    graphqlresponse)
endif()

# clientgen
if(GRAPHQL_BUILD_CLIENTGEN)
  add_executable(clientgen
    ClientGenerator.cpp
    RequestLoader.cpp)
  add_executable(cppgraphqlgen::clientgen ALIAS clientgen)
  target_link_libraries(clientgen PRIVATE generator_util)
  add_bigobj_flag(clientgen)

  # ClientGen.rc
  if(GRAPHQL_UPDATE_VERSION)
    add_dependencies(clientgen update_version_h)

    set(GRAPHQL_RC_FILENAME "clientgen")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Generator.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/ClientGen.rc
      @ONLY)

    add_custom_command(OUTPUT updated_clientgen_rc
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ClientGen.rc ${CMAKE_CURRENT_SOURCE_DIR}/../res
      COMMAND ${CMAKE_COMMAND} -E touch updated_clientgen_rc
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ClientGen.rc)

    add_custom_target(update_clientgen_rc ALL
      DEPENDS updated_clientgen_rc)
    add_dependencies(clientgen update_clientgen_rc)
  endif()

  if(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ClientGen.rc.res
      COMMAND ${CMAKE_RC_COMPILER} ${CMAKE_RC_FLAGS} $<$<CONFIG:Debug>:${CMAKE_RC_FLAGS_DEBUG}>
        "/fo${CMAKE_CURRENT_BINARY_DIR}/ClientGen.rc.res"
        "${CMAKE_CURRENT_SOURCE_DIR}/../res/ClientGen.rc"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../res/ClientGen.rc")
    add_custom_target(build_clientgen_rc ALL
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ClientGen.rc.res)

    add_library(clientgen_version OBJECT ${CMAKE_CURRENT_BINARY_DIR}/ClientGen.rc.res)
    set_target_properties(clientgen_version PROPERTIES LINKER_LANGUAGE CXX)
    add_dependencies(clientgen_version build_clientgen_rc)
    target_link_libraries(clientgen PRIVATE clientgen_version)
  endif()

  install(TARGETS clientgen
    EXPORT cppgraphqlgen-targets
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    RUNTIME DESTINATION ${GRAPHQL_INSTALL_TOOLS_DIR}/${PROJECT_NAME})
endif()

# schemagen
if(GRAPHQL_BUILD_SCHEMAGEN)
  add_executable(schemagen SchemaGenerator.cpp)
  add_executable(cppgraphqlgen::schemagen ALIAS schemagen)
  target_link_libraries(schemagen PRIVATE generator_util)
  add_bigobj_flag(schemagen)

  # SchemaGen.rc
  if(GRAPHQL_UPDATE_VERSION)
    add_dependencies(schemagen update_version_h)

    set(GRAPHQL_RC_FILENAME "schemagen")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Generator.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/SchemaGen.rc
      @ONLY)

    add_custom_command(OUTPUT updated_schemagen_rc
      COMMAND ${CMAKE_COMMAND} -E copy_if_different SchemaGen.rc ${CMAKE_CURRENT_SOURCE_DIR}/../res
      COMMAND ${CMAKE_COMMAND} -E touch updated_schemagen_rc
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/SchemaGen.rc)

    add_custom_target(update_schemagen_rc ALL
      DEPENDS updated_schemagen_rc)
    add_dependencies(schemagen update_schemagen_rc)
  endif()

  if(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SchemaGen.rc.res
      COMMAND ${CMAKE_RC_COMPILER} ${CMAKE_RC_FLAGS} $<$<CONFIG:Debug>:${CMAKE_RC_FLAGS_DEBUG}>
        "/fo${CMAKE_CURRENT_BINARY_DIR}/SchemaGen.rc.res"
        "${CMAKE_CURRENT_SOURCE_DIR}/../res/SchemaGen.rc"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../res/SchemaGen.rc")
    add_custom_target(build_schemagen_rc ALL
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/SchemaGen.rc.res)

    add_library(schemagen_version OBJECT ${CMAKE_CURRENT_BINARY_DIR}/SchemaGen.rc.res)
    set_target_properties(schemagen_version PROPERTIES LINKER_LANGUAGE CXX)
    add_dependencies(schemagen_version build_schemagen_rc)
    target_link_libraries(schemagen PRIVATE schemagen_version)
  endif()

  install(TARGETS schemagen
    EXPORT cppgraphqlgen-targets
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    RUNTIME DESTINATION ${GRAPHQL_INSTALL_TOOLS_DIR}/${PROJECT_NAME})
endif()

# Common schemagen and clientgen Boost dependencies
if(GRAPHQL_BUILD_SCHEMAGEN OR GRAPHQL_BUILD_CLIENTGEN)
  find_package(boost_program_options CONFIG REQUIRED)
  if(GRAPHQL_BUILD_SCHEMAGEN)
    target_link_libraries(schemagen PRIVATE Boost::program_options)
  endif()
  if(GRAPHQL_BUILD_CLIENTGEN)
    target_link_libraries(clientgen PRIVATE Boost::program_options)
  endif()
endif()

# introspection
add_subdirectory(introspection)
file(GLOB INTROSPECTION_SCHEMA_FILES ${CMAKE_CURRENT_SOURCE_DIR}/introspection/*.cpp)

# graphqlservice
add_library(graphqlservice
  GraphQLService.cpp
  Schema.cpp
  Validation.cpp
  Introspection.cpp
  ${INTROSPECTION_SCHEMA_FILES})
add_library(cppgraphqlgen::graphqlservice ALIAS graphqlservice)
target_compile_features(graphqlservice PUBLIC cxx_std_20)
target_link_libraries(graphqlservice PUBLIC
  graphql_internal_modules
  graphql_introspection_modules
  graphqlpeg
  graphqlresponse
  Threads::Threads)
target_sources(graphqlservice PUBLIC FILE_SET HEADERS
  BASE_DIRS ${INCLUDE_ROOT}
  FILES ${INCLUDE_ROOT}/graphqlservice/GraphQLService.h)
if(GRAPHQL_BUILD_MODULES)
  target_sources(graphqlservice PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${INCLUDE_ROOT}
    FILES ${INCLUDE_ROOT}/graphqlservice/Service.ixx)
endif()

if(GRAPHQL_UPDATE_SAMPLES)
  add_dependencies(graphqlservice copy_introspection_schema_headers)
endif()

if(GRAPHQL_UPDATE_VERSION)
  update_version_rc(graphqlservice)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlservice
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLSERVICE_DLL)

  add_version_rc(graphqlservice)
endif()

if(GRAPHQL_BUILD_CLIENTGEN)
  target_link_libraries(clientgen PRIVATE graphqlservice)
endif()

# graphqlclient
add_library(graphqlclient GraphQLClient.cpp)
add_library(cppgraphqlgen::graphqlclient ALIAS graphqlclient)
target_compile_features(graphqlclient PUBLIC cxx_std_20)
target_link_libraries(graphqlclient PUBLIC
  graphqlpeg
  graphqlresponse)
target_sources(graphqlclient PUBLIC FILE_SET HEADERS
  BASE_DIRS ${INCLUDE_ROOT}
  FILES ${INCLUDE_ROOT}/graphqlservice/GraphQLClient.h)
if(GRAPHQL_BUILD_MODULES)
  target_sources(graphqlclient PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${INCLUDE_ROOT}
    FILES ${INCLUDE_ROOT}/graphqlservice/Client.ixx)
endif()

if(GRAPHQL_UPDATE_VERSION)
  update_version_rc(graphqlclient)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlclient
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLCLIENT_DLL)

  add_version_rc(graphqlclient)
endif()

# RapidJSON is the only option for JSON serialization used in this project, but if you want
# to use another JSON library you can implement an alternate version of the functions in
# JSONResponse.cpp to serialize to and from GraphQLResponse and build graphqljson from that.
# You will also need to define how to build the graphqljson library target with your
# implementation, and you should set BUILD_GRAPHQLJSON so that the test dependencies know
# about your version of graphqljson.
if(GRAPHQL_USE_RAPIDJSON)
  find_package(RapidJSON CONFIG REQUIRED)

  set(BUILD_GRAPHQLJSON ON)
  add_library(graphqljson JSONResponse.cpp)
  add_library(cppgraphqlgen::graphqljson ALIAS graphqljson)
  target_compile_features(graphqljson PUBLIC cxx_std_20)
  target_link_libraries(graphqljson PUBLIC graphqlresponse)
  target_include_directories(graphqljson SYSTEM PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
  target_sources(graphqljson PUBLIC FILE_SET HEADERS
    BASE_DIRS ${INCLUDE_ROOT}
    FILES ${INCLUDE_ROOT}/graphqlservice/JSONResponse.h)
  if(GRAPHQL_BUILD_MODULES)
    target_sources(graphqljson PUBLIC FILE_SET CXX_MODULES
      BASE_DIRS ${INCLUDE_ROOT}
      FILES ${INCLUDE_ROOT}/graphqlservice/JSONResponse.ixx)
  endif()

  if(GRAPHQL_UPDATE_VERSION)
    update_version_rc(graphqljson)
  endif()

  if(WIN32 AND BUILD_SHARED_LIBS)
    target_compile_definitions(graphqljson
      PUBLIC GRAPHQL_DLLEXPORTS
      PRIVATE IMPL_JSONRESPONSE_DLL)

    add_version_rc(graphqljson)
  endif()
endif()

install(TARGETS
    graphql_internal_modules
    graphql_introspection_modules
    graphqlclient
    graphqlpeg
    graphqlresponse
    graphqlservice
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  FILE_SET HEADERS
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}
  FILE_SET CXX_MODULES
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR})

if(WIN32 AND BUILD_SHARED_LIBS)
  install(TARGETS
      graphqlpeg_version
      graphqlresponse_version
      graphqlservice_version
      graphqlclient_version
      graphqljson_version
    EXPORT cppgraphqlgen-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
endif()

# graphqljson
if(BUILD_GRAPHQLJSON)
  target_link_libraries(graphqljson PUBLIC graphqlresponse)

  install(TARGETS graphqljson
    EXPORT cppgraphqlgen-targets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    FILE_SET HEADERS
      CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
      DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}
    FILE_SET CXX_MODULES
      CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
      DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR})
else()
  set(GRAPHQL_BUILD_TESTS OFF CACHE BOOL "GRAPHQL_BUILD_TESTS depends on BUILD_GRAPHQLJSON" FORCE)
endif()

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})
