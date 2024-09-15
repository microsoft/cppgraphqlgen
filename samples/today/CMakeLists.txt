# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.28)

# todaygraphql
add_subdirectory(schema)
add_library(todaygraphql STATIC TodayMock.cpp)
target_compile_features(todaygraphql PUBLIC cxx_std_20)
target_link_libraries(todaygraphql PUBLIC today_schema)
target_sources(todaygraphql
  PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/TodayMock.h
  PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/TodayMock.ixx)

# todaygraphql_nointrospection
add_subdirectory(nointrospection)
add_library(todaygraphql_nointrospection STATIC TodayMock.cpp)
target_compile_features(todaygraphql_nointrospection PUBLIC cxx_std_20)
target_link_libraries(todaygraphql_nointrospection PUBLIC today_nointrospection_schema)
target_sources(todaygraphql_nointrospection
  PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/TodayMock.h
  PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/TodayMock.ixx)

if(MSVC)
  # warning C4702: unreachable code
  target_compile_options(todaygraphql PUBLIC /wd4702)
  target_compile_options(todaygraphql_nointrospection PUBLIC /wd4702)
endif()

# sample
add_executable(sample sample.cpp)
target_link_libraries(sample PRIVATE
  todaygraphql
  graphqljson)

# sample_nointrospection
add_executable(sample_nointrospection sample.cpp)
target_link_libraries(sample_nointrospection PRIVATE
  todaygraphql_nointrospection
  graphqljson)

# benchmark
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE
  todaygraphql
  graphqljson)

# benchmark_nointrospection
add_executable(benchmark_nointrospection benchmark.cpp)
target_link_libraries(benchmark_nointrospection PRIVATE
  todaygraphql_nointrospection
  graphqljson)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT copied_sample_dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:graphqlservice>
      $<TARGET_FILE:graphqljson>
      $<TARGET_FILE:graphqlpeg>
      $<TARGET_FILE:graphqlresponse>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch copied_sample_dlls
    DEPENDS
      graphqlservice
      graphqljson
      graphqlpeg
      graphqlresponse)

  add_custom_target(copy_today_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(sample copy_today_sample_dlls)
  add_dependencies(sample_nointrospection copy_today_sample_dlls)
  add_dependencies(benchmark copy_today_sample_dlls)
  add_dependencies(benchmark_nointrospection copy_today_sample_dlls)
endif()
