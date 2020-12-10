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
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --prefix="Today" --namespace="today"
    DEPENDS schemagen schema.today.graphql
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
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --prefix="Today" --namespace="today" --separate-files > today_schema_files
    DEPENDS schemagen schema.today.graphql
    WORKING_DIRECTORY separate
    COMMENT "Generating mock TodaySchema (--separate-files)")

  add_custom_target(separate_schema_files ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files)

  # unifiedschema
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/validation)

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.h
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.validation.graphql" --prefix="Validation" --namespace="validation"
    DEPENDS schemagen schema.validation.graphql
    WORKING_DIRECTORY validation
    COMMENT "Generating ValidationSchema files")

  add_custom_target(validation_schema_files ALL
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.h)

  file(GLOB OLD_UNIFIED_FILES ${CMAKE_CURRENT_SOURCE_DIR}/unified/*)
  file(GLOB OLD_SEPARATE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/separate/*)
  file(GLOB OLD_VALIDATION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/validation/ValidationSchema.*)

  add_custom_command(
    OUTPUT updated_samples
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_UNIFIED_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory unified/ ${CMAKE_CURRENT_SOURCE_DIR}/unified/
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_SEPARATE_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory separate/ ${CMAKE_CURRENT_SOURCE_DIR}/separate/
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_VALIDATION_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory validation/ ${CMAKE_CURRENT_SOURCE_DIR}/validation/
    COMMAND ${CMAKE_COMMAND} -E touch updated_samples
    DEPENDS 
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.h
    COMMENT "Updating sample files")

  add_custom_target(update_samples ALL
    DEPENDS updated_samples)
endif()

# sample
set(SEPARATE_SCHEMA_PATHS "")
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/separate/today_schema_files SEPARATE_SCHEMA_FILES)
foreach(CPP_FILE IN LISTS SEPARATE_SCHEMA_FILES)
  list(APPEND SEPARATE_SCHEMA_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/separate/${CPP_FILE}")
endforeach(CPP_FILE)

add_library(separateschema STATIC ${SEPARATE_SCHEMA_PATHS})
target_link_libraries(separateschema PUBLIC graphqlservice)
target_compile_definitions(separateschema PUBLIC IMPL_SEPARATE_TODAY)
target_include_directories(separateschema PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  separate)

if(GRAPHQL_UPDATE_SAMPLES)
  # wait for the sample update to complete
  add_dependencies(separateschema update_samples)
endif()

add_library(separategraphql STATIC today/TodayMock.cpp)
target_link_libraries(separategraphql PUBLIC separateschema)
target_compile_definitions(separategraphql PUBLIC IMPL_SEPARATE_TODAY)
target_include_directories(separategraphql PUBLIC today)

add_executable(sample today/sample.cpp)
target_link_libraries(sample PRIVATE
  separategraphql
  graphqljson)
target_include_directories(sample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_target(copy_sample_dlls ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlservice> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqljson> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlpeg> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlresponse> ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS graphqlservice graphqljson graphqlpeg graphqlresponse)

  add_dependencies(sample copy_sample_dlls)
endif()

# benchmark
add_executable(benchmark today/benchmark.cpp)
target_link_libraries(benchmark PRIVATE
  separategraphql
  graphqljson)
target_include_directories(benchmark PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_dependencies(benchmark copy_sample_dlls)
endif()

if(GRAPHQL_BUILD_TESTS)
  # tests
  add_library(unifiedschema STATIC unified/TodaySchema.cpp)
  target_link_libraries(unifiedschema PUBLIC graphqlservice)
  target_include_directories(unifiedschema PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    unified)

  if(GRAPHQL_UPDATE_SAMPLES)
    # wait for the sample update to complete
    add_dependencies(unifiedschema update_samples)
  endif()

  add_library(unifiedgraphql STATIC today/TodayMock.cpp)
  target_link_libraries(unifiedgraphql PUBLIC unifiedschema)
  target_include_directories(unifiedgraphql PUBLIC today)
  add_bigobj_flag(unifiedgraphql)

  add_library(validationgraphql STATIC
    validation/ValidationMock.cpp
    validation/ValidationSchema.cpp)
  target_link_libraries(validationgraphql PUBLIC graphqlservice)
  target_include_directories(validationgraphql PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    validation)
  add_bigobj_flag(validationgraphql)

  if(GRAPHQL_UPDATE_SAMPLES)
    # wait for the sample update to complete
    add_dependencies(validationgraphql update_samples)
  endif()
endif()
