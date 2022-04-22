# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

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
      add_library(${target}_version OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/../res/${target}_version.rc)
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

add_library(graphqlcoro INTERFACE)
target_compile_features(graphqlcoro INTERFACE cxx_std_20)

function(check_coroutine_impl COROUTINE_HEADER COROUTINE_NAMESPACE OPTIONAL_FLAGS OUT_RESULT)
  set(TEST_FILE "test_${OUT_RESULT}.cpp")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/test_coroutine.cpp.in ${TEST_FILE} @ONLY)

  try_compile(TEST_RESULT
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
    CXX_STANDARD 20)

  if(NOT TEST_RESULT)
      # Retry with each of the optional flags.
      foreach(OPTIONAL_FLAG IN LISTS OPTIONAL_FLAGS)
        try_compile(TEST_RESULT
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
          CMAKE_FLAGS "-DCOMPILE_DEFINITIONS:STRING=${OPTIONAL_FLAG}"
          CXX_STANDARD 20)

        if(TEST_RESULT)
          # Looks like the optional flag was required, go ahead and add it to the compile options.
          target_compile_options(graphqlcoro INTERFACE ${OPTIONAL_FLAG})
          break()
        endif()
      endforeach(OPTIONAL_FLAG)
  endif()

  set(${OUT_RESULT} ${TEST_RESULT} PARENT_SCOPE)
endfunction()

check_coroutine_impl("coroutine" "std" "-fcoroutines" STD_COROUTINE)
if(STD_COROUTINE)
  message(STATUS "Using std coroutine")
else()
  check_coroutine_impl("experimental/coroutine" "std::experimental" "" STD_EXPERIMENTAL_COROUTINE)
  if(STD_EXPERIMENTAL_COROUTINE)
    message(STATUS "Using std::experimental coroutine")
    target_compile_definitions(graphqlcoro INTERFACE USE_STD_EXPERIMENTAL_COROUTINE)
  else()
    message(FATAL_ERROR "Missing coroutine support")
  endif()
endif()

# graphqlpeg
add_library(graphqlpeg SyntaxTree.cpp)
add_library(cppgraphqlgen::graphqlpeg ALIAS graphqlpeg)
target_link_libraries(graphqlpeg PUBLIC
  graphqlcoro
  taocpp::pegtl)
target_include_directories(graphqlpeg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include>
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
target_include_directories(graphqlresponse PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
target_link_libraries(graphqlresponse PUBLIC graphqlcoro)

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
    add_library(clientgen_version OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/../res/ClientGen.rc)
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
    add_library(schemagen_version OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/../res/SchemaGen.rc)
    target_link_libraries(schemagen PRIVATE schemagen_version)
  endif()

  install(TARGETS schemagen
    EXPORT cppgraphqlgen-targets
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    RUNTIME DESTINATION ${GRAPHQL_INSTALL_TOOLS_DIR}/${PROJECT_NAME})
endif()

# Common schemagen and clientgen filesystem and Boost dependencies
if(GRAPHQL_BUILD_SCHEMAGEN OR GRAPHQL_BUILD_CLIENTGEN)
  # Try compiling a test program with std::filesystem or one of its alternatives.
  function(check_filesystem_impl OPTIONAL_LIBS)
    try_compile(TEST_RESULT
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/test_filesystem.cpp
      CXX_STANDARD 20)

    if(NOT TEST_RESULT)
      # Retry with each of the optional libraries.
      foreach(OPTIONAL_LIB IN LISTS OPTIONAL_LIBS)
        try_compile(TEST_RESULT
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
          LINK_LIBRARIES ${OPTIONAL_LIB}
          CXX_STANDARD 20)

        if(TEST_RESULT)
          # Looks like the optional library was required, go ahead and add it to the link options.
          if(GRAPHQL_BUILD_SCHEMAGEN)
            target_link_libraries(schemagen PRIVATE ${OPTIONAL_LIB})
          endif()
          if(GRAPHQL_BUILD_CLIENTGEN)
            target_link_libraries(clientgen PRIVATE ${OPTIONAL_LIB})
          endif()
          break()
        endif()
      endforeach(OPTIONAL_LIB)
    endif()
  endfunction(check_filesystem_impl)

  # Try compiling a minimal program without any extra libraries, then with each optional library until it succeeded:
  #   stdc++fs
  #   c++fs
  check_filesystem_impl("stdc++fs;c++fs" STD_FILESYTEM)

  find_package(Boost REQUIRED COMPONENTS program_options)
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
target_link_libraries(graphqlservice PUBLIC
  graphqlpeg
  graphqlresponse
  Threads::Threads)

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
target_link_libraries(graphqlclient PUBLIC
  graphqlpeg
  graphqlresponse)

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
option(GRAPHQL_USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)

if(GRAPHQL_USE_RAPIDJSON)
  find_package(RapidJSON CONFIG REQUIRED)

  set(BUILD_GRAPHQLJSON ON)
  add_library(graphqljson JSONResponse.cpp)
  add_library(cppgraphqlgen::graphqljson ALIAS graphqljson)
  target_link_libraries(graphqljson PUBLIC graphqlresponse)
  target_include_directories(graphqljson SYSTEM PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

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
    graphqlclient
    graphqlpeg
    graphqlcoro
    graphqlresponse
    graphqlservice
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLParse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLResponse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLService.h
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Awaitable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Base64.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Grammar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Introspection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Schema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/SortedMap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/SyntaxTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Version.h
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice/internal)

# graphqljson
if(BUILD_GRAPHQLJSON)
  option(GRAPHQL_BUILD_TESTS "Build the tests and sample schema library." ON)

  target_link_libraries(graphqljson PUBLIC graphqlresponse)

  install(TARGETS graphqljson
    EXPORT cppgraphqlgen-targets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/JSONResponse.h
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice)
else()
  set(GRAPHQL_BUILD_TESTS OFF CACHE BOOL "GRAPHQL_BUILD_TESTS depends on BUILD_GRAPHQLJSON" FORCE)
endif()

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})
