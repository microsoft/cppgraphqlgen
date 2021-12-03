# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

add_subdirectory(query)
add_subdirectory(mutate)
add_subdirectory(subscribe)

add_subdirectory(benchmark)

# client_benchmark
add_executable(client_benchmark benchmark.cpp)
target_link_libraries(client_benchmark PRIVATE
  separategraphql
  benchmark_client)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT copied_sample_dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:graphqlservice>
      $<TARGET_FILE:graphqlintrospection>
      $<TARGET_FILE:graphqljson>
      $<TARGET_FILE:graphqlpeg>
      $<TARGET_FILE:graphqlresponse>
      $<TARGET_FILE:graphqlclient>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/copied_sample_dlls
    DEPENDS
      graphqlservice
      graphqlintrospection
      graphqljson
      graphqlpeg
      graphqlresponse
      graphqlclient
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../src)

  add_custom_target(copy_client_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(client_benchmark copy_client_sample_dlls)
endif()
