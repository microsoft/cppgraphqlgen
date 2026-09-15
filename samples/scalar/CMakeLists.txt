# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# scalargraphql demonstrates a custom scalar (BigInt) whose C++ storage is a dedicated type
# (bigint::BigInt) instead of response::Value. See schema.scalar.graphql and BigIntScalar.h. The
# custom scalar (de)serialization in BigIntScalar.cpp is compiled into the scalar_schema library
# (see schema/CMakeLists.txt), so this is just a friendly alias target for the sample.
add_subdirectory(schema)

add_library(scalargraphql INTERFACE)
target_link_libraries(scalargraphql INTERFACE scalar_schema)
target_include_directories(scalargraphql INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
