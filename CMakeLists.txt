# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)
project(cppgraphqlgen VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)

set(GRAPHQL_INSTALL_INCLUDE_DIR include CACHE PATH "Header file install directory")
set(GRAPHQL_INSTALL_TOOLS_DIR bin CACHE PATH "schemagen install directory")
set(GRAPHQL_INSTALL_CMAKE_DIR lib/cmake CACHE PATH "CMake config files install directory")

option(BUILD_SHARED_LIBS "Build shared libraries instead of static libs" OFF)

if(WIN32 AND BUILD_SHARED_LIBS)
  # Let CMake figure out the exports for the SHARED library (DLL) on Windows.
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

function(add_bigobj_flag target)
  if(MSVC)
    # MSVC requires the /bigobj flag if the number of sections gets too big.
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()

find_package(Threads MODULE REQUIRED)

find_package(pegtl 3.0.0 QUIET CONFIG)
if(NOT pegtl_FOUND)
  # If a compatible version of PEGTL is not already installed, build and install it from the submodule directory.
  set(PEGTL_BUILD_TESTS OFF CACHE BOOL "Disable PEGTL tests")
  set(PEGTL_BUILD_EXAMPLES OFF CACHE BOOL "Disable PEGTL examples")
  set(PEGTL_INSTALL_INCLUDE_DIR ${GRAPHQL_INSTALL_INCLUDE_DIR} CACHE STRING "Override PEGTL include install directory")
  set(PEGTL_INSTALL_CMAKE_DIR ${GRAPHQL_INSTALL_CMAKE_DIR}/pegtl CACHE STRING "Override PEGTL cmake install directory")
  add_subdirectory(PEGTL)
endif()

# graphqlpeg
add_library(graphqlpeg GraphQLTree.cpp)
target_link_libraries(graphqlpeg PUBLIC taocpp::pegtl)
target_include_directories(graphqlpeg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PEGTL/include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
add_bigobj_flag(graphqlpeg)

# graphqlresponse
add_library(graphqlresponse OBJECT GraphQLResponse.cpp)
target_include_directories(graphqlresponse PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

# schemagen
add_executable(schemagen
  $<TARGET_OBJECTS:graphqlresponse>
  SchemaGenerator.cpp)
target_link_libraries(schemagen PRIVATE graphqlpeg)

set(BOOST_COMPONENTS program_options)
set(BOOST_LIBRARIES Boost::program_options)

if(NOT MSVC)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem)
  set(BOOST_LIBRARIES ${BOOST_LIBRARIES} Boost::filesystem)
  target_compile_options(schemagen PRIVATE -DUSE_BOOST_FILESYSTEM)
endif()

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
target_link_libraries(schemagen PRIVATE ${BOOST_LIBRARIES})

add_custom_command(
  OUTPUT
    IntrospectionSchema.cpp
    include/graphqlservice/IntrospectionSchema.h
  COMMAND schemagen --introspection
  DEPENDS schemagen
  COMMENT "Generating IntrospectionSchema files"
)

# graphqlservice
add_library(graphqlservice
  $<TARGET_OBJECTS:graphqlresponse>
  GraphQLService.cpp
  Introspection.cpp
  IntrospectionSchema.cpp)
target_link_libraries(graphqlservice PUBLIC
  graphqlpeg
  Threads::Threads)
target_include_directories(graphqlservice PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/include)

# RapidJSON is the only option for JSON serialization used in this project, but if you want
# to use another JSON library you can implement an alternate version of the functions in
# JSONResponse.cpp to serialize to and from GraphQLResponse and build graphqljson from that.
# You will also need to define how to build the graphqljson library target with your
# implementation, and you should set BUILD_GRAPHQLJSON so that the test dependencies know
# about your version of graphqljson.
option(USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)

if(USE_RAPIDJSON)
  find_package(RapidJSON CONFIG REQUIRED)

  set(BUILD_GRAPHQLJSON ON)
  add_library(graphqljson JSONResponse.cpp)
  target_include_directories(graphqljson SYSTEM PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
endif()

# graphqljson
if(BUILD_GRAPHQLJSON)
  option(BUILD_TESTS "Build the tests and sample schema library." ON)

  target_link_libraries(graphqljson PUBLIC graphqlservice)

  install(TARGETS graphqljson
    EXPORT cppgraphqlgen-targets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
  install(FILES include/graphqlservice/JSONResponse.h
    DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice
    CONFIGURATIONS Release)
else()
  set(BUILD_TESTS OFF)
endif()

option(UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests." ON)

if(BUILD_TESTS OR UPDATE_SAMPLES)
  # todaygraphql
  add_custom_command(
    OUTPUT
      samples/today/TodaySchema.cpp
      samples/today/TodaySchema.h
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/samples/today/schema.today.graphql" --prefix="Today" --namespace="today"
    DEPENDS schemagen samples/today/schema.today.graphql
    WORKING_DIRECTORY samples/today
    COMMENT "Generating mock TodaySchema files"
  )

  # separate_today
  add_custom_command(
    OUTPUT samples/separate_today/today_schema_files
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/samples/today/schema.today.graphql" --prefix="Today" --namespace="today" --separate-files > today_schema_files
    DEPENDS schemagen samples/today/schema.today.graphql
    WORKING_DIRECTORY samples/separate_today
    COMMENT "Generating mock TodaySchema separate files"
  )

  # force the generation of samples on the default build target
  add_custom_target(update_samples ALL
    DEPENDS
      samples/today/TodaySchema.cpp
      samples/separate_today/today_schema_files
  )

  if(BUILD_TESTS)
    add_library(todaygraphql
      samples/today/Today.cpp
      samples/today/TodaySchema.cpp)
    target_link_libraries(todaygraphql PUBLIC
      graphqlservice)
    target_include_directories(todaygraphql PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/include
      include
      PEGTL/include
      samples/today)

    file(STRINGS samples/separate_today/today_schema_files SEPARATE_SCHEMA_CPP)
    list(TRANSFORM SEPARATE_SCHEMA_CPP PREPEND samples/separate_today/)

    add_library(separategraphql
      samples/separate_today/Today.cpp
      ${SEPARATE_SCHEMA_CPP})
    target_link_libraries(separategraphql PUBLIC
      graphqlservice)
    target_include_directories(separategraphql PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/include
      include
      PEGTL/include
      samples/separate_today)

    # test_today
    add_executable(test_today
      test_today.cpp)
    target_link_libraries(test_today PRIVATE
      separategraphql
      graphqljson)

    # tests
    enable_testing()
    find_package(GTest MODULE REQUIRED)

    add_executable(tests
      tests.cpp)
    target_link_libraries(tests PRIVATE
      todaygraphql
      graphqljson
      GTest::GTest
      GTest::Main)

    add_test(NAME TodayServiceCase
      COMMAND tests --gtest_filter=TodayServiceCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME ArgumentsCase
      COMMAND tests --gtest_filter=ArgumentsCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME PegtlCase
      COMMAND tests --gtest_filter=PegtlCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME ResponseCase
      COMMAND tests --gtest_filter=ResponseCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
  endif()

  if(UPDATE_SAMPLES)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
        ${CMAKE_CURRENT_BINARY_DIR}/IntrospectionSchema.cpp
      DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/samples/introspection)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/samples
      DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endif()

install(TARGETS
    graphqlpeg
    graphqlservice
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(TARGETS schemagen
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION ${GRAPHQL_INSTALL_TOOLS_DIR}/${PROJECT_NAME}
  CONFIGURATIONS Release)

install(FILES
    include/graphqlservice/GraphQLTree.h
    include/graphqlservice/GraphQLResponse.h
    include/graphqlservice/GraphQLService.h
    include/graphqlservice/Introspection.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice
  CONFIGURATIONS Release)

# Enable version checks in find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(cmake/${PROJECT_NAME}-config-version.cmake COMPATIBILITY SameMajorVersion)

install(FILES
    cmake/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME}
)

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME}
)
