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
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --prefix="Today" --namespace="today" --stubs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      TodaySchema.cpp
      TodaySchema.h
      ${CMAKE_CURRENT_SOURCE_DIR}/unified
    DEPENDS schemagen graphqlpeg schema.today.graphql
    WORKING_DIRECTORY unified
    COMMENT "Generating mock TodaySchema files (--stubs)")

  # unifiedschema_nointrospection
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unified_nointrospection)

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/unified_nointrospection/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified_nointrospection/TodaySchema.h
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --prefix="Today" --namespace="today" --stubs --no-introspection
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      TodaySchema.cpp
      TodaySchema.h
      ${CMAKE_CURRENT_SOURCE_DIR}/unified_nointrospection
    DEPENDS schemagen graphqlpeg schema.today.graphql
    WORKING_DIRECTORY unified_nointrospection
    COMMENT "Generating mock TodaySchema files without Introspection (--stubs --no-introspection)")

  # validationschema
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/validation)

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.h
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.validation.graphql" --prefix="Validation" --namespace="validation" --stubs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ValidationSchema.cpp
      ValidationSchema.h
      ${CMAKE_CURRENT_SOURCE_DIR}/validation
    DEPENDS schemagen graphqlpeg schema.validation.graphql
    WORKING_DIRECTORY validation
    COMMENT "Generating ValidationSchema files (--stubs)")

  # separateschema
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/separate)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --prefix="Today" --namespace="today" --separate-files --stubs > today_schema_files
    COMMAND ${CMAKE_COMMAND} -E copy_if_different today_schema_files ${CMAKE_CURRENT_SOURCE_DIR}/separate
    DEPENDS schemagen graphqlpeg schema.today.graphql
    WORKING_DIRECTORY separate
    COMMENT "Generating mock TodaySchema (--stubs --separate-files)")

  # separateschema_nointrospection
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/separate_nointrospection)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/separate_nointrospection/today_schema_files
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --prefix="Today" --namespace="today" --no-introspection --separate-files --stubs > today_schema_files
    COMMAND ${CMAKE_COMMAND} -E copy_if_different today_schema_files ${CMAKE_CURRENT_SOURCE_DIR}/separate_nointrospection
    DEPENDS schemagen graphqlpeg schema.today.graphql
    WORKING_DIRECTORY separate_nointrospection
    COMMENT "Generating mock TodaySchema without Introspection (--stubs --separate-files --no-introspection)")

  # learn
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/learn)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/learn/learn_schema_files
    COMMAND schemagen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.learn.graphql" --prefix="StarWars" --namespace="learn" --separate-files > learn_schema_files
    COMMAND ${CMAKE_COMMAND} -E copy_if_different learn_schema_files ${CMAKE_CURRENT_SOURCE_DIR}/learn
    DEPENDS schemagen graphqlpeg schema.learn.graphql
    WORKING_DIRECTORY learn
    COMMENT "Generating mock StarWarsSchema (--separate-files)")

  add_custom_command(
    OUTPUT updated_samples
    COMMAND ${CMAKE_COMMAND}
      "-DSCHEMA_SOURCE_LIST=today_schema_files"
      "-DSCHEMA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/separate"
      "-DSCHEMA_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/separate"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/update_samples.cmake"
    COMMAND ${CMAKE_COMMAND}
      "-DSCHEMA_SOURCE_LIST=today_schema_files"
      "-DSCHEMA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/separate_nointrospection"
      "-DSCHEMA_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/separate_nointrospection"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/update_samples.cmake"
    COMMAND ${CMAKE_COMMAND}
      "-DSCHEMA_SOURCE_LIST=learn_schema_files"
      "-DSCHEMA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/learn"
      "-DSCHEMA_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/learn"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/update_samples.cmake"
    COMMAND ${CMAKE_COMMAND} -E touch updated_samples
    DEPENDS 
      ${CMAKE_CURRENT_BINARY_DIR}/separate/today_schema_files
      ${CMAKE_CURRENT_BINARY_DIR}/separate_nointrospection/today_schema_files
      ${CMAKE_CURRENT_BINARY_DIR}/learn/learn_schema_files
    COMMENT "Updating sample files")

  add_custom_target(update_samples ALL
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified/TodaySchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/unified_nointrospection/TodaySchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/unified_nointrospection/TodaySchema.h
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/validation/ValidationSchema.h
      updated_samples)

  if(GRAPHQL_BUILD_CLIENTGEN)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/client)

    # query.today.graphql
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/client/QueryClient.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/client/QueryClient.h
      COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/query.today.graphql" --prefix="Query" --namespace="query"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        QueryClient.cpp
        QueryClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client
      DEPENDS clientgen schema.today.graphql query.today.graphql
      WORKING_DIRECTORY client
      COMMENT "Generating QueryClient samples")

    # mutate.today.graphql
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/client/MutateClient.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/client/MutateClient.h
      COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/mutate.today.graphql" --prefix="Mutate" --namespace="mutate"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        MutateClient.cpp
        MutateClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client
      DEPENDS clientgen schema.today.graphql mutate.today.graphql
      WORKING_DIRECTORY client
      COMMENT "Generating MutateClient samples")

    # subscribe.today.graphql
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/client/SubscribeClient.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/client/SubscribeClient.h
      COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/subscribe.today.graphql" --prefix="Subscribe" --namespace="subscribe"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        SubscribeClient.cpp
        SubscribeClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client
      DEPENDS clientgen schema.today.graphql subscribe.today.graphql
      WORKING_DIRECTORY client
      COMMENT "Generating SubscribeClient samples")

    # client.benchmark.today.graphql
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/client/BenchmarkClient.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/client/BenchmarkClient.h
      COMMAND clientgen --schema="${CMAKE_CURRENT_SOURCE_DIR}/schema.today.graphql" --request="${CMAKE_CURRENT_SOURCE_DIR}/client.benchmark.today.graphql" --prefix="Benchmark" --namespace="benchmark"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        BenchmarkClient.cpp
        BenchmarkClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client
      DEPENDS clientgen schema.today.graphql client.benchmark.today.graphql
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
endif()

# separateschema
set(SEPARATE_SCHEMA_PATHS "")
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/separate/today_schema_files SEPARATE_SCHEMA_FILES)
foreach(CPP_FILE IN LISTS SEPARATE_SCHEMA_FILES)
  list(APPEND SEPARATE_SCHEMA_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/separate/${CPP_FILE}")
endforeach(CPP_FILE)

add_library(separateschema STATIC ${SEPARATE_SCHEMA_PATHS})
target_link_libraries(separateschema PUBLIC graphqlintrospection)
target_compile_definitions(separateschema PUBLIC IMPL_SEPARATE_TODAY)
target_include_directories(separateschema PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/separate)

if(GRAPHQL_UPDATE_SAMPLES)
  # wait for the sample update to complete
  add_dependencies(separateschema update_samples)
endif()

# separateschema_nointrospection
set(SEPARATE_NOINTROSPECTION_SCHEMA_PATHS "")
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/separate_nointrospection/today_schema_files SEPARATE_NOINTROSPECTION_SCHEMA_FILES)
foreach(CPP_FILE IN LISTS SEPARATE_NOINTROSPECTION_SCHEMA_FILES)
  list(APPEND SEPARATE_NOINTROSPECTION_SCHEMA_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/separate_nointrospection/${CPP_FILE}")
endforeach(CPP_FILE)

add_library(separateschema_nointrospection STATIC ${SEPARATE_NOINTROSPECTION_SCHEMA_PATHS})
target_link_libraries(separateschema_nointrospection PUBLIC graphqlservice)
target_compile_definitions(separateschema_nointrospection PUBLIC IMPL_SEPARATE_TODAY)
target_include_directories(separateschema_nointrospection PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/separate_nointrospection)

if(GRAPHQL_UPDATE_SAMPLES)
  # wait for the sample update to complete
  add_dependencies(separateschema_nointrospection update_samples)
endif()

# learnschema
set(LEARN_SCHEMA_PATHS "")
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/learn/learn_schema_files LEARN_SCHEMA_FILES)
foreach(CPP_FILE IN LISTS LEARN_SCHEMA_FILES)
  list(APPEND LEARN_SCHEMA_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/learn/${CPP_FILE}")
endforeach(CPP_FILE)

add_library(learnschema STATIC ${LEARN_SCHEMA_PATHS})
target_link_libraries(learnschema PUBLIC graphqlintrospection)
target_include_directories(learnschema PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/learn)

if(GRAPHQL_UPDATE_SAMPLES)
  # wait for the sample update to complete
  add_dependencies(learnschema update_samples)
endif()

# separategraphql
add_library(separategraphql STATIC today/TodayMock.cpp)
target_link_libraries(separategraphql PUBLIC separateschema)
target_include_directories(separategraphql PUBLIC today)

# separategraphql_nointrospection
add_library(separategraphql_nointrospection STATIC today/TodayMock.cpp)
target_link_libraries(separategraphql_nointrospection PUBLIC separateschema_nointrospection)
target_include_directories(separategraphql_nointrospection PUBLIC today)

# star_wars
add_library(star_wars STATIC
  star_wars/DroidData.cpp
  star_wars/HumanData.cpp
  star_wars/QueryData.cpp
  star_wars/ReviewData.cpp
  star_wars/MutationData.cpp
  star_wars/StarWarsData.cpp)
target_link_libraries(star_wars PUBLIC learnschema)
target_include_directories(star_wars PUBLIC star_wars)

# sample
add_executable(sample today/sample.cpp)
target_link_libraries(sample PRIVATE
  separategraphql
  graphqljson)
target_include_directories(sample PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

# sample_nointrospection
add_executable(sample_nointrospection today/sample.cpp)
target_link_libraries(sample_nointrospection PRIVATE
  separategraphql_nointrospection
  graphqljson)
target_include_directories(sample_nointrospection PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

# learn_star_wars
add_executable(learn_star_wars star_wars/sample.cpp)
target_link_libraries(learn_star_wars PRIVATE
  star_wars
  graphqljson)
target_include_directories(learn_star_wars PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

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
    COMMAND ${CMAKE_COMMAND} -E touch copied_sample_dlls
    DEPENDS
      graphqlservice
      graphqlintrospection
      graphqljson
      graphqlpeg
      graphqlresponse
      graphqlclient)

  add_custom_target(copy_sample_dlls DEPENDS copied_sample_dlls)

  add_dependencies(sample copy_sample_dlls)
  add_dependencies(sample_nointrospection copy_sample_dlls)
endif()

# benchmark
add_executable(benchmark today/benchmark.cpp)
target_link_libraries(benchmark PRIVATE
  separategraphql
  graphqljson)
target_include_directories(benchmark PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

# benchmark_nointrospection
add_executable(benchmark_nointrospection today/benchmark.cpp)
target_link_libraries(benchmark_nointrospection PRIVATE
  separategraphql_nointrospection
  graphqljson)
target_include_directories(benchmark_nointrospection PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_dependencies(benchmark copy_sample_dlls)
  add_dependencies(benchmark_nointrospection copy_sample_dlls)
endif()

if(GRAPHQL_BUILD_TESTS)
  # tests
  add_library(unifiedschema STATIC ${CMAKE_CURRENT_SOURCE_DIR}/unified/TodaySchema.cpp)
  target_link_libraries(unifiedschema PUBLIC graphqlintrospection)
  target_include_directories(unifiedschema PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/unified)

  if(GRAPHQL_UPDATE_SAMPLES)
    # wait for the sample update to complete
    add_dependencies(unifiedschema update_samples)
  endif()

  add_library(unifiedgraphql STATIC today/TodayMock.cpp)
  target_link_libraries(unifiedgraphql PUBLIC unifiedschema)
  target_include_directories(unifiedgraphql PUBLIC today)
  add_bigobj_flag(unifiedgraphql)

  add_library(unifiedschema_nointrospection STATIC ${CMAKE_CURRENT_SOURCE_DIR}/unified_nointrospection/TodaySchema.cpp)
  target_link_libraries(unifiedschema_nointrospection PUBLIC graphqlservice)
  target_include_directories(unifiedschema_nointrospection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/unified_nointrospection)

  if(GRAPHQL_UPDATE_SAMPLES)
    # wait for the sample update to complete
    add_dependencies(unifiedschema_nointrospection update_samples)
  endif()

  add_library(unifiedgraphql_nointrospection STATIC today/TodayMock.cpp)
  target_link_libraries(unifiedgraphql_nointrospection PUBLIC unifiedschema_nointrospection)
  target_include_directories(unifiedgraphql_nointrospection PUBLIC today)
  add_bigobj_flag(unifiedgraphql_nointrospection)

  add_library(validationgraphql STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/validation/ValidationMock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/validation/ValidationSchema.cpp)
  target_link_libraries(validationgraphql PUBLIC graphqlintrospection)
  target_include_directories(validationgraphql PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/validation)
  add_bigobj_flag(validationgraphql)

  if(GRAPHQL_UPDATE_SAMPLES)
    # wait for the sample update to complete
    add_dependencies(validationgraphql update_samples)
  endif()
endif()

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
