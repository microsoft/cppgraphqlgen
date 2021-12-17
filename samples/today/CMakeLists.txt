# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

# separategraphql
add_subdirectory(separate)
add_library(separategraphql STATIC TodayMock.cpp)
target_link_libraries(separategraphql PUBLIC separate_schema)
target_include_directories(separategraphql PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(separategraphql PUBLIC IMPL_SEPARATE_TODAY)

# separategraphql_nointrospection
add_subdirectory(separate_nointrospection)
add_library(separategraphql_nointrospection STATIC TodayMock.cpp)
target_link_libraries(separategraphql_nointrospection PUBLIC separate_nointrospection_schema)
target_include_directories(separategraphql_nointrospection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(separategraphql_nointrospection PUBLIC IMPL_SEPARATE_TODAY)

# sample
add_executable(sample sample.cpp)
target_link_libraries(sample PRIVATE
  separategraphql
  graphqljson)

# sample_nointrospection
add_executable(sample_nointrospection sample.cpp)
target_link_libraries(sample_nointrospection PRIVATE
  separategraphql_nointrospection
  graphqljson)

# benchmark
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE
  separategraphql
  graphqljson)

# benchmark_nointrospection
add_executable(benchmark_nointrospection benchmark.cpp)
target_link_libraries(benchmark_nointrospection PRIVATE
  separategraphql_nointrospection
  graphqljson)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT copied_sample_dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:graphqlservice>
      $<TARGET_FILE:graphqlintrospection>
      $<TARGET_FILE:graphqljson>
      $<TARGET_FILE:graphqlpeg>
      $<TARGET_FILE:graphqlresponse>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch copied_sample_dlls
    DEPENDS
      graphqlservice
      graphqlintrospection
      graphqljson
      graphqlpeg
      graphqlresponse)

  add_custom_target(copy_today_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(sample copy_today_sample_dlls)
  add_dependencies(sample_nointrospection copy_today_sample_dlls)
  add_dependencies(benchmark copy_today_sample_dlls)
  add_dependencies(benchmark_nointrospection copy_today_sample_dlls)
endif()

if(GRAPHQL_BUILD_TESTS)
  # tests
  add_subdirectory(unified)
  add_library(unifiedgraphql STATIC TodayMock.cpp)
  target_link_libraries(unifiedgraphql PUBLIC unified_schema)
  target_include_directories(unifiedgraphql PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  add_bigobj_flag(unifiedgraphql)

  add_subdirectory(unified_nointrospection)
  add_library(unifiedgraphql_nointrospection STATIC TodayMock.cpp)
  target_link_libraries(unifiedgraphql_nointrospection PUBLIC unified_nointrospection_schema)
  target_include_directories(unifiedgraphql_nointrospection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  add_bigobj_flag(unifiedgraphql_nointrospection)
endif()
