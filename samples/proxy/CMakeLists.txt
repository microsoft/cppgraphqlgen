# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

add_subdirectory(schema)
add_subdirectory(client)

add_executable(proxy proxy.cpp)
target_link_libraries(proxy PRIVATE
  proxy_schema
  proxyrelay_client
  graphqljson)
target_include_directories(proxy PRIVATE
  client
  schema)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT copied_sample_dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:graphqlservice>
      $<TARGET_FILE:graphqljson>
      $<TARGET_FILE:graphqlpeg>
      $<TARGET_FILE:graphqlresponse>
      $<TARGET_FILE:graphqlclient>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch copied_sample_dlls
    DEPENDS
      graphqlservice
      graphqljson
      graphqlpeg
      graphqlresponse
      graphqlclient)

  add_custom_target(copy_proxy_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(proxy copy_proxy_sample_dlls)
endif()
