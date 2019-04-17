# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

# todaygraphql
add_custom_command(
  OUTPUT
    today/TodaySchema.cpp
    today/TodaySchema.h
  COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today"
  DEPENDS schemagen today/schema.today.graphql
  WORKING_DIRECTORY today
  COMMENT "Generating mock TodaySchema files"
)

# separate_today
add_custom_command(
  OUTPUT separate_today/today_schema_files
  COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today" --separate-files > today_schema_files
  DEPENDS schemagen today/schema.today.graphql
  WORKING_DIRECTORY separate_today
  COMMENT "Generating mock TodaySchema (--separate-files)"
)

# force the generation of samples on the default build target
add_custom_target(update_samples ALL
  DEPENDS
    today/TodaySchema.cpp
    separate_today/today_schema_files
)

if(GRAPHQL_BUILD_TESTS)
  add_library(todaygraphql
    today/Today.cpp
    today/TodaySchema.cpp)
  target_link_libraries(todaygraphql PUBLIC
    graphqlservice)
  target_include_directories(todaygraphql PUBLIC
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/PEGTL/include
    ${CMAKE_CURRENT_BINARY_DIR}/today
    today)
endif()

file(STRINGS separate_today/today_schema_files SEPARATE_SCHEMA_CPP)
list(TRANSFORM SEPARATE_SCHEMA_CPP PREPEND separate_today/)

add_library(separategraphql
  separate_today/Today.cpp
  ${SEPARATE_SCHEMA_CPP})
target_link_libraries(separategraphql PUBLIC
  graphqlservice)
target_include_directories(separategraphql PUBLIC
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/PEGTL/include
  ${CMAKE_CURRENT_BINARY_DIR}/separate_today
  separate_today)

# test_today
add_executable(test_today
  separate_today/sample.cpp)
target_link_libraries(test_today PRIVATE
  separategraphql
  graphqljson)

if(GRAPHQL_UPDATE_SAMPLES)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/today
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})

  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/separate_today
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
endif()
