# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

# Get the up-to-date list of files in the binary directory
set(FILE_NAMES "")
file(GLOB NEW_FILES ${SCHEMA_BINARY_DIR}/*.h ${SCHEMA_BINARY_DIR}/*.cpp)
foreach(NEW_FILE ${NEW_FILES})
  get_filename_component(NEW_FILE ${NEW_FILE} NAME)
  list(APPEND FILE_NAMES "${NEW_FILE}")
endforeach()

# Don't update the files in the source directory if no files were generated in the binary directory.
if(NOT FILE_NAMES)
  message(FATAL_ERROR "Schema generation failed!")
endif()

# Remove stale files in the source directory
file(GLOB OLD_FILES ${SCHEMA_SOURCE_DIR}/*.h ${SCHEMA_SOURCE_DIR}/*.cpp)
foreach(OLD_FILE ${OLD_FILES})
  get_filename_component(OLD_FILE ${OLD_FILE} NAME)
  if(NOT OLD_FILE IN_LIST FILE_NAMES)
    file(REMOVE "${SCHEMA_SOURCE_DIR}/${OLD_FILE}")
  endif()
endforeach()

# Copy new and modified files from the binary directory to the source directory
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SCHEMA_BINARY_DIR}/${SCHEMA_SOURCE_LIST} ${NEW_FILES} ${SCHEMA_SOURCE_DIR})
