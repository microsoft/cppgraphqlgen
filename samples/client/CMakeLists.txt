# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.28)

add_subdirectory(query)
add_subdirectory(mutate)
add_subdirectory(subscribe)
add_subdirectory(nestedinput)
add_subdirectory(multiple)

add_subdirectory(benchmark)

if(GRAPHQL_BUILD_MODULES)
  # client_benchmark
  add_executable(client_benchmark benchmark.cpp)
  target_link_libraries(client_benchmark PRIVATE
    benchmark_client)

  if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(OUTPUT copied_sample_dlls
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:graphqlservice>
        $<TARGET_FILE:graphqljson>
        $<TARGET_FILE:graphqlpeg>
        $<TARGET_FILE:graphqlresponse>
        $<TARGET_FILE:graphqlclient>
        ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/copied_sample_dlls
      DEPENDS
        graphqlservice
        graphqljson
        graphqlpeg
        graphqlresponse
        graphqlclient)

    add_custom_target(copy_client_sample_dlls DEPENDS copied_sample_dlls)

    add_dependencies(client_benchmark copy_client_sample_dlls)
  endif()
endif()
