# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

find_package(GTest MODULE REQUIRED)

add_executable(validation_tests ValidationTests.cpp)
target_link_libraries(validation_tests PRIVATE
  validationgraphql
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(validation_tests)
gtest_add_tests(TARGET validation_tests)

add_executable(today_tests TodayTests.cpp)
target_link_libraries(today_tests PRIVATE
  todaygraphql
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(today_tests)
gtest_add_tests(TARGET today_tests)

add_executable(coroutine_tests CoroutineTests.cpp)
target_link_libraries(coroutine_tests PRIVATE
  todaygraphql
  graphqljson
  GTest::GTest
  GTest::Main)
gtest_add_tests(TARGET coroutine_tests)

add_executable(client_tests ClientTests.cpp)
target_link_libraries(client_tests PRIVATE
  todaygraphql
  query_client
  mutate_client
  subscribe_client
  GTest::GTest
  GTest::Main)
add_bigobj_flag(client_tests)
gtest_add_tests(TARGET client_tests)

add_executable(nointrospection_tests NoIntrospectionTests.cpp)
target_link_libraries(nointrospection_tests PRIVATE
  todaygraphql_nointrospection
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(nointrospection_tests)
gtest_add_tests(TARGET nointrospection_tests)

add_executable(argument_tests ArgumentTests.cpp)
target_link_libraries(argument_tests PRIVATE
  todaygraphql
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(argument_tests)
gtest_add_tests(TARGET argument_tests)

add_executable(pegtl_combined_tests PegtlCombinedTests.cpp)
target_link_libraries(pegtl_combined_tests PRIVATE
  taocpp::pegtl
  GTest::GTest
  GTest::Main)
target_include_directories(pegtl_combined_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
gtest_add_tests(TARGET pegtl_combined_tests)

add_executable(pegtl_executable_tests PegtlExecutableTests.cpp)
target_link_libraries(pegtl_executable_tests PRIVATE
  taocpp::pegtl
  GTest::GTest
  GTest::Main)
target_include_directories(pegtl_executable_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
gtest_add_tests(TARGET pegtl_executable_tests)

add_executable(pegtl_schema_tests PegtlSchemaTests.cpp)
target_link_libraries(pegtl_schema_tests PRIVATE
  taocpp::pegtl
  GTest::GTest
  GTest::Main)
target_include_directories(pegtl_schema_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
gtest_add_tests(TARGET pegtl_schema_tests)

add_executable(response_tests ResponseTests.cpp)
target_link_libraries(response_tests PRIVATE
  graphqlservice
  GTest::GTest
  GTest::Main)
target_include_directories(response_tests PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
gtest_add_tests(TARGET response_tests)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT copied_test_dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:graphqlservice>
      $<TARGET_FILE:graphqljson>
      $<TARGET_FILE:graphqlpeg>
      $<TARGET_FILE:graphqlresponse>
      $<TARGET_FILE:graphqlclient>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/copied_test_dlls
    DEPENDS
      graphqlservice
      graphqljson
      graphqlpeg
      graphqlresponse
      graphqlclient)

  add_custom_target(copy_test_dlls DEPENDS copied_test_dlls)

  add_dependencies(validation_tests copy_test_dlls)
  add_dependencies(today_tests copy_test_dlls)
  add_dependencies(client_tests copy_test_dlls)
  add_dependencies(nointrospection_tests copy_test_dlls)
  add_dependencies(argument_tests copy_test_dlls)
  add_dependencies(pegtl_combined_tests copy_test_dlls)
  add_dependencies(pegtl_executable_tests copy_test_dlls)
  add_dependencies(pegtl_schema_tests copy_test_dlls)
  add_dependencies(response_tests copy_test_dlls)
endif()
