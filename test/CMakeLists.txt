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
