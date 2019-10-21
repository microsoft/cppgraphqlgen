# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unified)

# todaygraphql
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/unified/TodaySchema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unified/TodaySchema.h
  COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today"
  DEPENDS schemagen today/schema.today.graphql
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unified
  COMMENT "Generating mock TodaySchema files")

# separate
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/separate/today_schema_files SEPARATE_SCHEMA_CPP)
list(TRANSFORM SEPARATE_SCHEMA_CPP PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/separate/)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/separate)

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/separate/today_schema_files
    ${CMAKE_CURRENT_SOURCE_DIR}/separate/TodayObjects.h
    ${CMAKE_CURRENT_SOURCE_DIR}/separate/TodaySchema.h
    ${SEPARATE_SCHEMA_CPP}
  COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today" --separate-files > today_schema_files
  DEPENDS schemagen today/schema.today.graphql
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/separate
  COMMENT "Generating mock TodaySchema (--separate-files)")

# force the generation of samples on the default build target
add_custom_target(update_samples ALL
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/unified/TodaySchema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unified/TodaySchema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/separate/today_schema_files
    ${CMAKE_CURRENT_SOURCE_DIR}/separate/TodayObjects.h
    ${CMAKE_CURRENT_SOURCE_DIR}/separate/TodaySchema.h
    ${SEPARATE_SCHEMA_CPP})

if(GRAPHQL_BUILD_TESTS)
  add_library(unifiedschema OBJECT unified/TodaySchema.cpp)
  target_link_libraries(unifiedschema PUBLIC graphqlservice)
  target_include_directories(unifiedschema PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/unified)
  add_bigobj_flag(unifiedschema)

  add_library(unifiedgraphql today/UnifiedToday.cpp)
  target_link_libraries(unifiedgraphql PUBLIC unifiedschema)
  target_include_directories(unifiedgraphql PUBLIC today)
endif()

add_library(separateschema OBJECT ${SEPARATE_SCHEMA_CPP})
target_link_libraries(separateschema PUBLIC graphqlservice)
target_include_directories(separateschema PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/separate)

add_library(separategraphql today/SeparateToday.cpp)
target_link_libraries(separategraphql PUBLIC separateschema)
target_include_directories(separategraphql PUBLIC today)

# test_today
add_executable(sample today/sample.cpp)
target_link_libraries(sample PRIVATE
  separategraphql
  graphqljson)
target_include_directories(sample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/separate
  ${CMAKE_CURRENT_SOURCE_DIR}/today)
