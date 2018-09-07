# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.5.1)
project(cppgraphql)

if(WIN32)
  # Let CMake figure out the exports for the SHARED library (DLL) on Windows.
  SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

add_library(graphqlservice SHARED GraphQLService.cpp Introspection.cpp IntrospectionSchema.cpp)
add_executable(schemagen SchemaGenerator.cpp)

find_library(GRAPHQLPARSER graphqlparser)

if(UNIX)
  find_library(CPPRESTSDK_LIB cpprest)
elseif(WIN32)
  find_package(cpprestsdk REQUIRED)
  set(CPPRESTSDK_LIB cpprestsdk::cpprest)
endif()

target_include_directories(graphqlservice SYSTEM PUBLIC ${CMAKE_PREFIX_PATH}/include ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(graphqlservice ${CPPRESTSDK_LIB} ${GRAPHQLPARSER})
target_link_libraries(schemagen ${CPPRESTSDK_LIB} ${GRAPHQLPARSER})

add_custom_command(
  OUTPUT IntrospectionSchema.cpp IntrospectionSchema.h
  COMMAND schemagen
  DEPENDS schemagen
  COMMENT "Generating IntrospectionSchema files"
)

add_custom_command(
  OUTPUT TodaySchema.cpp TodaySchema.h
  COMMAND schemagen ${CMAKE_SOURCE_DIR}/schema.today.graphql Today today
  DEPENDS schemagen schema.today.graphql
  COMMENT "Generating mock TodaySchema files"
)

add_library(todaygraphql SHARED
  TodaySchema.cpp
  Today.cpp)

target_link_libraries(todaygraphql
  ${CPPRESTSDK_LIB}
  ${GRAPHQLPARSER}
  graphqlservice)
target_include_directories(todaygraphql SYSTEM PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})

add_executable(test_today test_today.cpp)

target_link_libraries(test_today
  ${CPPRESTSDK_LIB}
  ${GRAPHQLPARSER}
  graphqlservice
  todaygraphql)
target_include_directories(test_today SYSTEM PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})

enable_testing()
add_executable(tests tests.cpp)
find_package(GTest REQUIRED)
target_link_libraries(tests
  ${CPPRESTSDK_LIB}
  ${GRAPHQLPARSER}
  graphqlservice
  todaygraphql
  GTest::GTest
  GTest::Main)
target_include_directories(tests SYSTEM PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
add_test(TodayServiceCase tests)
add_test(ArgumentsCase tests)

if(UNIX)
  target_compile_options(graphqlservice PRIVATE -std=c++11)
  target_compile_options(todaygraphql PRIVATE -std=c++11)
  target_compile_options(schemagen PRIVATE -std=c++11)
  target_compile_options(test_today PRIVATE -std=c++11)
  target_compile_options(tests PRIVATE -std=c++11)
endif()

install(TARGETS graphqlservice schemagen
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)

install(FILES GraphQLService.h Introspection.h IntrospectionSchema.h
  DESTINATION include/graphqlservice)

install(FILES IntrospectionSchema.h IntrospectionSchema.cpp TodaySchema.h TodaySchema.cpp
  DESTINATION ${CMAKE_SOURCE_DIR}/samples)

if(WIN32)
  set(GRAPHQLSERVICE_DIR $<TARGET_FILE_DIR:graphqlservice>)
  install(FILES ${GRAPHQLSERVICE_DIR}/graphqlservice.lib
    DESTINATION lib)

  option(INSTALL_WINDOWS_DLLS "Copy all Windows DLLs as part of the install to get dependencies." OFF)

  if(INSTALL_WINDOWS_DLLS)
    install(CODE "include(${CMAKE_SOURCE_DIR}/copy-windows-dlls.cmake)"
      CODE "copy_windows_dlls(${CMAKE_BINARY_DIR})")
  endif()
endif()
