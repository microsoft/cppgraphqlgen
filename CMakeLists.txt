# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.5.1)
project(cppgraphqlgen)

if(WIN32)
  # Let CMake figure out the exports for the SHARED library (DLL) on Windows.
  SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

find_package(Threads MODULE REQUIRED)

find_package(pegtl CONFIG REQUIRED)

add_executable(schemagen
  GraphQLTree.cpp
  GraphQLResponse.cpp
  SchemaGenerator.cpp)
target_link_libraries(schemagen PRIVATE taocpp::pegtl)
target_include_directories(schemagen PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
set_property(TARGET schemagen PROPERTY CXX_STANDARD 11)

add_custom_command(
  OUTPUT IntrospectionSchema.cpp IntrospectionSchema.h
  COMMAND schemagen
  DEPENDS schemagen
  COMMENT "Generating IntrospectionSchema files"
)

add_library(graphqlservice
  GraphQLTree.cpp
  GraphQLResponse.cpp
  GraphQLService.cpp
  Introspection.cpp
  IntrospectionSchema.cpp)
target_link_libraries(graphqlservice PRIVATE taocpp::pegtl)
target_link_libraries(graphqlservice PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(graphqlservice SYSTEM INTERFACE $<INSTALL_INTERFACE:include>)
target_include_directories(graphqlservice PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
set_property(TARGET graphqlservice PROPERTY CXX_STANDARD 11)

option(BUILD_TESTS "Build the tests and sample schema library." ON)
option(UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests and the sample library." ON)

if(BUILD_TESTS OR UPDATE_SAMPLES)
  add_custom_command(
    OUTPUT TodaySchema.cpp TodaySchema.h
    COMMAND schemagen ${CMAKE_SOURCE_DIR}/schema.today.graphql Today today
    DEPENDS schemagen schema.today.graphql
    COMMENT "Generating mock TodaySchema files"
  )

  # force the generation of samples on the default build target
  add_custom_target(update_samples ALL
    DEPENDS ${CMAKE_BINARY_DIR}/TodaySchema.cpp
  )

  if(BUILD_TESTS)
    find_package(RapidJSON CONFIG REQUIRED)

    add_library(todaygraphql
       ${CMAKE_BINARY_DIR}/TodaySchema.cpp
      Today.cpp
	  JSONResponse.cpp)
    target_link_libraries(todaygraphql PUBLIC
      graphqlservice)
    target_include_directories(todaygraphql SYSTEM PUBLIC ${RAPIDJSON_INCLUDE_DIRS})
    target_include_directories(todaygraphql PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    set_property(TARGET todaygraphql PROPERTY CXX_STANDARD 11)

    add_executable(test_today
      test_today.cpp)
    target_link_libraries(test_today PRIVATE
      todaygraphql)
    target_include_directories(test_today PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    set_property(TARGET test_today PROPERTY CXX_STANDARD 11)

    enable_testing()
    find_package(GTest MODULE REQUIRED)

    add_executable(tests
      tests.cpp)
    target_link_libraries(tests PRIVATE
      todaygraphql
      GTest::GTest
      GTest::Main)
    target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    set_property(TARGET tests PROPERTY CXX_STANDARD 11)

    add_test(NAME TodayServiceCase
      COMMAND tests --gtest_filter=TodayServiceCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME ArgumentsCase
      COMMAND tests --gtest_filter=ArgumentsCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
    add_test(NAME PegtlCase
      COMMAND tests --gtest_filter=PegtlCase.*
      WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
  endif()

  if(UPDATE_SAMPLES)
    install(FILES
      ${CMAKE_BINARY_DIR}/IntrospectionSchema.h
      ${CMAKE_BINARY_DIR}/IntrospectionSchema.cpp
      ${CMAKE_BINARY_DIR}/TodaySchema.h
      ${CMAKE_BINARY_DIR}/TodaySchema.cpp
      DESTINATION ${CMAKE_SOURCE_DIR}/samples)
  endif()
endif()

install(TARGETS graphqlservice
  EXPORT cppgraphqlgen-config
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(TARGETS schemagen
  RUNTIME DESTINATION tools/cppgraphqlgen
  CONFIGURATIONS Release)

install(FILES
  GraphQLTree.h
  GraphQLResponse.h
  GraphQLService.h
  Introspection.h
  ${CMAKE_BINARY_DIR}/IntrospectionSchema.h
  JSONResponse.h
  DESTINATION include/graphqlservice
  CONFIGURATIONS Release)

install(EXPORT cppgraphqlgen-config
  NAMESPACE cppgraphqlgen::
  DESTINATION share/cppgraphqlgen)
