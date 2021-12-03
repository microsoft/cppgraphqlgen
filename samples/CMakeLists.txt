# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

if(GRAPHQL_UPDATE_SAMPLES AND GRAPHQL_BUILD_CLIENTGEN)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/client)

  # query.today.graphql
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/client/QueryClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/QueryClient.h
    COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/query.today.graphql" --prefix="Query" --namespace="query"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      QueryClient.cpp
      QueryClient.h
      ${CMAKE_CURRENT_SOURCE_DIR}/client
    DEPENDS clientgen today/schema.today.graphql query.today.graphql
    WORKING_DIRECTORY client
    COMMENT "Generating QueryClient samples")

  # mutate.today.graphql
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/client/MutateClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/MutateClient.h
    COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/mutate.today.graphql" --prefix="Mutate" --namespace="mutate"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      MutateClient.cpp
      MutateClient.h
      ${CMAKE_CURRENT_SOURCE_DIR}/client
    DEPENDS clientgen today/schema.today.graphql mutate.today.graphql
    WORKING_DIRECTORY client
    COMMENT "Generating MutateClient samples")

  # subscribe.today.graphql
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/client/SubscribeClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/SubscribeClient.h
    COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/subscribe.today.graphql" --prefix="Subscribe" --namespace="subscribe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      SubscribeClient.cpp
      SubscribeClient.h
      ${CMAKE_CURRENT_SOURCE_DIR}/client
    DEPENDS clientgen today/schema.today.graphql subscribe.today.graphql
    WORKING_DIRECTORY client
    COMMENT "Generating SubscribeClient samples")

  # client.benchmark.today.graphql
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/client/BenchmarkClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/BenchmarkClient.h
    COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/client.benchmark.today.graphql" --prefix="Benchmark" --namespace="benchmark"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      BenchmarkClient.cpp
      BenchmarkClient.h
      ${CMAKE_CURRENT_SOURCE_DIR}/client
    DEPENDS clientgen today/schema.today.graphql client.benchmark.today.graphql
    WORKING_DIRECTORY client
    COMMENT "Generating BenchmarkClient samples")

  add_custom_target(update_client_samples ALL
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/client/QueryClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/QueryClient.h
      ${CMAKE_CURRENT_BINARY_DIR}/client/MutateClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/MutateClient.h
      ${CMAKE_CURRENT_BINARY_DIR}/client/SubscribeClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/SubscribeClient.h
      ${CMAKE_CURRENT_BINARY_DIR}/client/BenchmarkClient.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/client/BenchmarkClient.h)
endif()

add_subdirectory(learn)
add_subdirectory(today)
add_subdirectory(validation)

# todayclient
add_library(todayclient STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/client/QueryClient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/client/MutateClient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/client/SubscribeClient.cpp)
target_link_libraries(todayclient PUBLIC graphqlclient)
target_include_directories(todayclient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/client)

if(GRAPHQL_UPDATE_SAMPLES AND GRAPHQL_BUILD_CLIENTGEN)
  # wait for the sample update to complete
  add_dependencies(todayclient update_client_samples)
endif()

# client_benchmark
add_executable(client_benchmark
  ${CMAKE_CURRENT_SOURCE_DIR}/client/BenchmarkClient.cpp
  client/benchmark.cpp)
target_link_libraries(client_benchmark PRIVATE
  separategraphql
  todayclient
  graphqlclient)
target_include_directories(client_benchmark PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/client)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_dependencies(client_benchmark copy_sample_dlls)
endif()
