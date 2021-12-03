# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

# Enable version checks in find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${PROJECT_NAME}-config-version.cmake COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-functions.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-update-schema-files.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-update-client-files.cmake
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})
