# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.5.1)
project(cppgraphqlgen)

if(WIN32)
  # Let CMake figure out the exports for the SHARED library (DLL) on Windows.
  SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

add_library(graphqlpeg GraphQLTree.cpp)
add_library(graphqlservice GraphQLService.cpp Introspection.cpp IntrospectionSchema.cpp)
add_executable(schemagen SchemaGenerator.cpp)

find_package(pegtl CONFIG REQUIRED)

target_include_directories(graphqlpeg INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}> $<INSTALL_INTERFACE:include>)
target_link_libraries(graphqlpeg taocpp::pegtl)

find_package(cpprestsdk REQUIRED)
set(CPPRESTSDK cpprestsdk::cpprest)

if(UNIX)
  find_library(BOOST_SYSTEM boost_system)
  list(APPEND CPPRESTSDK ${BOOST_SYSTEM})
endif()

target_include_directories(graphqlservice INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}> $<INSTALL_INTERFACE:include>)
target_link_libraries(graphqlservice ${CPPRESTSDK} graphqlpeg)
target_link_libraries(schemagen ${CPPRESTSDK} graphqlpeg)

add_custom_command(
  OUTPUT IntrospectionSchema.cpp IntrospectionSchema.h
  COMMAND schemagen
  DEPENDS schemagen
  COMMENT "Generating IntrospectionSchema files"
)

if(UNIX)
  target_compile_options(graphqlpeg PRIVATE -std=c++11)
  target_compile_options(graphqlservice PRIVATE -std=c++11)
  target_compile_options(schemagen PRIVATE -std=c++11)
endif()

option(BUILD_TESTS "Build the tests and sample schema library." ON)
option(UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests and the sample library." ON)

if(BUILD_TESTS OR UPDATE_SAMPLES)
  add_custom_command(
    OUTPUT TodaySchema.cpp TodaySchema.h
    COMMAND schemagen ${CMAKE_SOURCE_DIR}/schema.today.graphql Today today
    DEPENDS schemagen schema.today.graphql
    COMMENT "Generating mock TodaySchema files"
  )

  if(BUILD_TESTS)
    add_library(todaygraphql
      TodaySchema.cpp
      Today.cpp)

    target_link_libraries(todaygraphql
      ${CPPRESTSDK}
      graphqlservice)
    target_include_directories(todaygraphql SYSTEM PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})

    add_executable(test_today test_today.cpp)

    target_link_libraries(test_today
      ${CPPRESTSDK}
	  graphqlpeg
      graphqlservice
      todaygraphql)
    target_include_directories(test_today SYSTEM PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})

    enable_testing()
    add_executable(tests tests.cpp)
    find_package(GTest REQUIRED)
    target_link_libraries(tests
      ${CPPRESTSDK}
	  graphqlpeg
      graphqlservice
      todaygraphql
      GTest::GTest
      GTest::Main)
    target_include_directories(tests SYSTEM PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
    add_test(TodayServiceCase tests)
    add_test(ArgumentsCase tests)

    if(UNIX)
      target_compile_options(todaygraphql PRIVATE -std=c++11)
      target_compile_options(test_today PRIVATE -std=c++11)
      target_compile_options(tests PRIVATE -std=c++11)
    endif()
  endif()

  if(UPDATE_SAMPLES)
    install(FILES IntrospectionSchema.h IntrospectionSchema.cpp TodaySchema.h TodaySchema.cpp
      DESTINATION ${CMAKE_SOURCE_DIR}/samples)
  endif()
endif()

install(TARGETS graphqlpeg graphqlservice schemagen
  EXPORT cppgraphqlgen-config
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(FILES GraphQLService.h GraphQLTree.h Introspection.h IntrospectionSchema.h
  DESTINATION include/graphqlservice)

if(WIN32)
  set(GRAPHQLSERVICE_DIR $<TARGET_FILE_DIR:graphqlservice>)
  install(FILES ${GRAPHQLSERVICE_DIR}/graphqlservice.lib  ${GRAPHQLSERVICE_DIR}/graphqlpeg.lib
    DESTINATION lib)
endif()

install(EXPORT cppgraphqlgen-config
  NAMESPACE cppgraphqlgen::
  DESTINATION lib/cppgraphqlgen)
