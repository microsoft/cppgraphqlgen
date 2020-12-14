# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

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
  unifiedgraphql
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(today_tests)
gtest_add_tests(TARGET today_tests)

add_executable(nointrospection_tests NoIntrospectionTests.cpp)
target_link_libraries(nointrospection_tests PRIVATE
  unifiedgraphql_nointrospection
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(nointrospection_tests)
gtest_add_tests(TARGET nointrospection_tests)

add_executable(argument_tests ArgumentTests.cpp)
target_link_libraries(argument_tests PRIVATE
  unifiedgraphql
  graphqljson
  GTest::GTest
  GTest::Main)
add_bigobj_flag(argument_tests)
gtest_add_tests(TARGET argument_tests)

add_executable(pegtl_tests PegtlTests.cpp)
target_link_libraries(pegtl_tests PRIVATE
  GTest::GTest
  GTest::Main)
target_include_directories(pegtl_tests PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include)
gtest_add_tests(TARGET pegtl_tests)

add_executable(response_tests ResponseTests.cpp)
target_link_libraries(response_tests PRIVATE
  graphqlservice
  GTest::GTest
  GTest::Main)
target_include_directories(response_tests PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
gtest_add_tests(TARGET response_tests)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_target(copy_test_dlls ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlservice_nointrospection> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlservice> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqljson> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlpeg> ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:graphqlresponse> ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS graphqlservice graphqljson graphqlpeg graphqlresponse
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../src)

  add_dependencies(validation_tests copy_test_dlls)
  add_dependencies(today_tests copy_test_dlls)
  add_dependencies(nointrospection_tests copy_test_dlls)
  add_dependencies(argument_tests copy_test_dlls)
  add_dependencies(pegtl_tests copy_test_dlls)
  add_dependencies(response_tests copy_test_dlls)
endif()
