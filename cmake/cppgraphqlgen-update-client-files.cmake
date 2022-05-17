# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Normalize the filesystem paths
get_filename_component(CLIENT_SOURCE_DIR ${CLIENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(CLIENT_BINARY_DIR ${CLIENT_BINARY_DIR} ABSOLUTE)
get_filename_component(CLIENTGEN_PROGRAM ${CLIENTGEN_PROGRAM} ABSOLUTE)
get_filename_component(SCHEMA_GRAPHQL "${CLIENT_SOURCE_DIR}/${SCHEMA_GRAPHQL}" ABSOLUTE)
get_filename_component(REQUEST_GRAPHQL "${CLIENT_SOURCE_DIR}/${REQUEST_GRAPHQL}" ABSOLUTE)

file(MAKE_DIRECTORY ${CLIENT_BINARY_DIR})

# Cleanup all of the stale files in the binary directory
file(GLOB PREVIOUS_FILES ${CLIENT_BINARY_DIR}/*.h ${CLIENT_BINARY_DIR}/*.cpp
     ${CLIENT_BINARY_DIR}/${CLIENT_TARGET}_client_files)
foreach(PREVIOUS_FILE ${PREVIOUS_FILES})
  file(REMOVE ${PREVIOUS_FILE})
endforeach()

set(CLIENTGEN_ARGS "--schema=${SCHEMA_GRAPHQL}" "--request=${REQUEST_GRAPHQL}" "--prefix=${CLIENT_PREFIX}" "--namespace=${CLIENT_NAMESPACE}")
foreach(CLIENTGEN_ARG ${ADDITIONAL_CLIENTGEN_ARGS})
  list(APPEND CLIENTGEN_ARGS ${CLIENTGEN_ARG})
endforeach()

# Regenerate the sources in the binary directory
execute_process(
  COMMAND ${CLIENTGEN_PROGRAM} ${CLIENTGEN_ARGS}
  OUTPUT_FILE ${CLIENT_TARGET}_client_files
  WORKING_DIRECTORY ${CLIENT_BINARY_DIR})

# Get the up-to-date list of files in the binary directory
set(FILE_NAMES "")
file(GLOB NEW_FILES ${CLIENT_BINARY_DIR}/*.h ${CLIENT_BINARY_DIR}/*.cpp)
foreach(NEW_FILE ${NEW_FILES})
  get_filename_component(NEW_FILE ${NEW_FILE} NAME)
  list(APPEND FILE_NAMES "${NEW_FILE}")
endforeach()

# Don't update the files in the source directory if no files were generated in the binary directory.
if(NOT FILE_NAMES)
  message(FATAL_ERROR "Client generation failed!")
endif()

 # Support if() IN_LIST operator: https://cmake.org/cmake/help/latest/policy/CMP0057.html
cmake_policy(SET CMP0057 NEW)

# Remove stale files in the source directory
file(GLOB OLD_FILES ${CLIENT_SOURCE_DIR}/*.h ${CLIENT_SOURCE_DIR}/*.cpp)
foreach(OLD_FILE ${OLD_FILES})
  get_filename_component(OLD_FILE ${OLD_FILE} NAME)
  if(NOT OLD_FILE IN_LIST FILE_NAMES)
    if(OLD_FILE MATCHES "Client\\.h$" OR OLD_FILE MATCHES "Client\\.cpp$")
      file(REMOVE "${CLIENT_SOURCE_DIR}/${OLD_FILE}")
    else()
      message(WARNING "Unexpected file in ${CLIENT_TARGET} client sources: ${OLD_FILE}")
    endif()
  endif()
endforeach()

# Copy new and modified files from the binary directory to the source directory
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CLIENT_BINARY_DIR}/${CLIENT_TARGET}_client_files
    ${NEW_FILES}
  ${CLIENT_SOURCE_DIR})
