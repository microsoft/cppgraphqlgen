# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

add_subdirectory(schema)
add_library(star_wars STATIC
  HeroData.cpp
  DroidData.cpp
  HumanData.cpp
  QueryData.cpp
  ReviewData.cpp
  MutationData.cpp
  StarWarsData.cpp)
target_link_libraries(star_wars PUBLIC learn_schema)
target_include_directories(star_wars PUBLIC star_wars)

add_executable(learn_star_wars sample.cpp)
target_link_libraries(learn_star_wars PRIVATE
  star_wars
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

  add_custom_target(copy_learn_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(learn_star_wars copy_learn_sample_dlls)
endif()
