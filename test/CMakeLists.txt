# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

if(GRAPHQL_BUILD_TESTS)
  # tests
  find_package(GTest MODULE REQUIRED)

  add_executable(tests
    tests.cpp)
  target_link_libraries(tests PRIVATE
    todaygraphql
    graphqljson
    GTest::GTest
    GTest::Main)

  add_test(NAME TodayServiceCase
    COMMAND tests --gtest_filter=TodayServiceCase.*
    WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
  add_test(NAME ArgumentsCase
    COMMAND tests --gtest_filter=ArgumentsCase.*
    WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
  add_test(NAME PegtlCase
    COMMAND tests --gtest_filter=PegtlCase.*
    WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
  add_test(NAME ResponseCase
    COMMAND tests --gtest_filter=ResponseCase.*
    WORKING_DIRECTORY $<TARGET_FILE_DIR:tests>)
endif()
