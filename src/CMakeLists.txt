# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

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

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Version.rc.in 
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
  # Always build the version.rc files with these flags, even if we aren't regnerating them.
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

# graphqlpeg
add_library(graphqlpeg GraphQLTree.cpp)
add_library(cppgraphqlgen::graphqlpeg ALIAS graphqlpeg)
target_link_libraries(graphqlpeg PUBLIC taocpp::pegtl)
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

# schemagen
if(GRAPHQL_BUILD_SCHEMAGEN)
  add_executable(schemagen SchemaGenerator.cpp)
  add_executable(cppgraphqlgen::schemagen ALIAS schemagen)
  target_link_libraries(schemagen PRIVATE generator_util)
  add_bigobj_flag(schemagen)

  # SchemaGen.rc
  if(GRAPHQL_UPDATE_VERSION)
    add_dependencies(schemagen update_version_h)

    set(SCHEMAGEN_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    set(SCHEMAGEN_RC_VERSION_STR "${PROJECT_VERSION}")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/SchemaGen.rc.in 
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
  
    set(CLIENTGEN_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    set(CLIENTGEN_RC_VERSION_STR "${PROJECT_VERSION}")
  
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClientGen.rc.in 
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

# Common schemagen and clientgen filesystem and Boost dependencies
if(GRAPHQL_BUILD_SCHEMAGEN OR GRAPHQL_BUILD_CLIENTGEN)
  set(BOOST_COMPONENTS program_options)
  set(BOOST_LIBRARIES Boost::program_options)

  # Try compiling a test program with std::filesystem or one of its alternatives.
  function(check_filesystem_impl FILESYSTEM_HEADER FILESYSTEM_NAMESPACE OPTIONAL_LIBS OUT_RESULT)
    set(TEST_FILE "test_${OUT_RESULT}.cpp")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/test_filesystem.cpp.in ${TEST_FILE} @ONLY)

    try_compile(TEST_RESULT
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
      CXX_STANDARD 17)

    if(NOT TEST_RESULT)
      # Retry with each of the optional libraries.
      foreach(OPTIONAL_LIB IN LISTS OPTIONAL_LIBS)
        try_compile(TEST_RESULT
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
          LINK_LIBRARIES ${OPTIONAL_LIB}
          CXX_STANDARD 17)

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

    set(${OUT_RESULT} ${TEST_RESULT} PARENT_SCOPE)
  endfunction(check_filesystem_impl)

  # Try compiling a minimal program with each header/namespace, in order of preference:
  #   C++17: #include <filesystem> // std::filesystem
  #   Experimental C++17: #include <experimental/filesystem> // std::experimental::filesystem
  #   Boost.Filesystem: #include <boost/filesystem.hpp> // boost::filesystem
  check_filesystem_impl("filesystem" "std::filesystem" "stdc++fs;c++fs" STD_FILESYTEM)
  if(STD_FILESYTEM)
    if(GRAPHQL_BUILD_SCHEMAGEN)
      target_compile_definitions(schemagen PRIVATE USE_STD_FILESYSTEM)
    endif()
    if(GRAPHQL_BUILD_CLIENTGEN)
      target_compile_definitions(clientgen PRIVATE USE_STD_FILESYSTEM)
    endif()
  else()
    check_filesystem_impl("experimental/filesystem" "std::experimental::filesystem" "stdc++fs;c++fs" STD_EXPERIMENTAL_FILESYTEM)
    if(STD_EXPERIMENTAL_FILESYTEM)
      if(GRAPHQL_BUILD_SCHEMAGEN)
        target_compile_definitions(schemagen PRIVATE USE_STD_EXPERIMENTAL_FILESYSTEM)
      endif()
      if(GRAPHQL_BUILD_CLIENTGEN)
        target_compile_definitions(clientgen PRIVATE USE_STD_EXPERIMENTAL_FILESYSTEM)
      endif()
    else()
      set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem)
      set(BOOST_LIBRARIES ${BOOST_LIBRARIES} Boost::filesystem)
      if(GRAPHQL_BUILD_SCHEMAGEN)
        target_compile_definitions(schemagen PRIVATE USE_BOOST_FILESYSTEM)
      endif()
      if(GRAPHQL_BUILD_CLIENTGEN)
        target_compile_definitions(clientgen PRIVATE USE_BOOST_FILESYSTEM)
      endif()
    endif()
  endif()

  find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
  if(GRAPHQL_BUILD_SCHEMAGEN)
    target_link_libraries(schemagen PRIVATE ${BOOST_LIBRARIES})
  endif()
  if(GRAPHQL_BUILD_CLIENTGEN)
    target_link_libraries(clientgen PRIVATE ${BOOST_LIBRARIES})
  endif()
endif()

# introspection
if(GRAPHQL_UPDATE_SAMPLES)
  add_custom_command(
    OUTPUT updated_introspection
    COMMAND schemagen --introspection
    COMMAND ${CMAKE_COMMAND} -E copy_if_different IntrospectionSchema.cpp ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different IntrospectionSchema.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/introspection
    COMMAND ${CMAKE_COMMAND} -E touch updated_introspection
    DEPENDS schemagen
    COMMENT "Updating IntrospectionSchema files")

  add_custom_target(update_introspection ALL
    DEPENDS updated_introspection)
endif()

# graphqlservice
add_library(graphqlservice
  GraphQLService.cpp
  GraphQLSchema.cpp
  Validation.cpp)
add_library(cppgraphqlgen::graphqlservice ALIAS graphqlservice)
target_link_libraries(graphqlservice PUBLIC
    graphqlpeg
    graphqlresponse
    Threads::Threads)

if(GRAPHQL_UPDATE_SAMPLES)
  # Even though this target doesn't build IntrospectionSchema.cpp, it still
  # depends on IntrospectionSchema.h for some enum definitions.
  add_dependencies(graphqlservice update_introspection)
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

# graphqlintrospection
add_library(graphqlintrospection
  Introspection.cpp
  IntrospectionSchema.cpp)
add_library(cppgraphqlgen::graphqlintrospection ALIAS graphqlintrospection)
target_link_libraries(graphqlintrospection PUBLIC graphqlservice)

if(GRAPHQL_UPDATE_VERSION)
  update_version_rc(graphqlintrospection)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlintrospection
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLINTROSPECTION_DLL)

  add_version_rc(graphqlintrospection)
endif()

if(GRAPHQL_BUILD_CLIENTGEN)
  target_link_libraries(clientgen PRIVATE graphqlintrospection)
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

# graphqljson
if(BUILD_GRAPHQLJSON)
  option(GRAPHQL_BUILD_TESTS "Build the tests and sample schema library." ON)

  target_link_libraries(graphqljson PUBLIC graphqlintrospection)

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

install(TARGETS
    graphqlpeg
    graphqlresponse
    graphqlservice
    graphqlintrospection
    graphqlclient
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLParse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLResponse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLSchema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLGrammar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLClient.h
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Base64.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/SortedMap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/internal/Version.h
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice/internal)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/introspection/Introspection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/introspection/IntrospectionSchema.h
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice/introspection)

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})
