# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.28)

add_library(stitchedschema STATIC
  StitchedSchema.cpp)
target_link_libraries(stitchedschema PUBLIC star_wars todaygraphql)
target_include_directories(stitchedschema INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(stitched sample.cpp)
target_link_libraries(stitched PRIVATE
  stitchedschema
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

  add_custom_target(copy_stitched_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(stitched copy_stitched_sample_dlls)
endif()
