# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

if(GRAPHQL_UPDATE_SAMPLES)
  # unifiedschema
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unified)

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.h
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today"
    DEPENDS schemagen today/schema.today.graphql
    WORKING_DIRECTORY unified
    COMMENT "Generating mock TodaySchema files")

  add_custom_target(unified_schema_files ALL
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.h)

  # separateschema
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/separate)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today" --separate-files > today_schema_files
    DEPENDS schemagen today/schema.today.graphql
    WORKING_DIRECTORY separate
    COMMENT "Generating mock TodaySchema (--separate-files)")

  add_custom_target(separate_schema_files ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files)

  file(GLOB OLD_INTROSPECTION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/introspection/*)
  file(GLOB OLD_UNIFIED_FILES ${CMAKE_CURRENT_SOURCE_DIR}/unified/*)
  file(GLOB OLD_SEPARATE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/separate/*)

  add_custom_command(
    OUTPUT updated_samples
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_INTROSPECTION_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy ../IntrospectionSchema.cpp ${CMAKE_CURRENT_SOURCE_DIR}/introspection/
    COMMAND ${CMAKE_COMMAND} -E copy ../include/graphqlservice/IntrospectionSchema.h ${CMAKE_CURRENT_SOURCE_DIR}/introspection/
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_UNIFIED_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory unified/ ${CMAKE_CURRENT_SOURCE_DIR}/unified/
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_SEPARATE_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory separate/ ${CMAKE_CURRENT_SOURCE_DIR}/separate/
    COMMAND ${CMAKE_COMMAND} -E touch updated_samples
    DEPENDS 
      ${CMAKE_CURRENT_BINARY_DIR}/../IntrospectionSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/../include/graphqlservice/IntrospectionSchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files
    COMMENT "Updating sample files")

  add_custom_target(update_samples ALL
    DEPENDS updated_samples)
endif()

# sample
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/separate/today_schema_files SEPARATE_SCHEMA_CPP)
list(TRANSFORM SEPARATE_SCHEMA_CPP PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/separate/)

add_library(separateschema OBJECT ${SEPARATE_SCHEMA_CPP})
target_link_libraries(separateschema PUBLIC graphqlservice)
target_include_directories(separateschema PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  separate)

if(GRAPHQL_UPDATE_SAMPLES)
  # wait for the sample update to complete
  add_dependencies(separateschema update_samples)
endif()

add_library(separategraphql today/SeparateToday.cpp)
target_link_libraries(separategraphql PUBLIC separateschema)
target_include_directories(separategraphql PUBLIC today)

add_executable(sample today/sample.cpp)
target_link_libraries(sample PRIVATE
  separategraphql
  graphqljson)
target_include_directories(sample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

if(GRAPHQL_BUILD_TESTS)
  # tests
  add_library(unifiedschema OBJECT unified/TodaySchema.cpp)
  target_link_libraries(unifiedschema PUBLIC graphqlservice)
  target_include_directories(unifiedschema PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    unified)
  add_bigobj_flag(unifiedschema)

  if(GRAPHQL_UPDATE_SAMPLES)
    # wait for the sample update to complete
    add_dependencies(unifiedschema update_samples)
  endif()

  add_library(unifiedgraphql today/UnifiedToday.cpp)
  target_link_libraries(unifiedgraphql PUBLIC unifiedschema)
  target_include_directories(unifiedgraphql PUBLIC today)
endif()
