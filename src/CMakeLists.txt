# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)

function(add_bigobj_flag target)
  if(MSVC)
    # MSVC requires the /bigobj flag if the number of sections gets too big.
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()

# graphqlpeg
add_library(graphqlpeg GraphQLTree.cpp)
target_link_libraries(graphqlpeg PUBLIC taocpp::pegtl)
target_include_directories(graphqlpeg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/PEGTL/include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
add_bigobj_flag(graphqlpeg)

# graphqlresponse
add_library(graphqlresponse OBJECT GraphQLResponse.cpp)
target_include_directories(graphqlresponse PRIVATE
  ${CMAKE_SOURCE_DIR}/include)

# schemagen
add_executable(schemagen
  $<TARGET_OBJECTS:graphqlresponse>
  SchemaGenerator.cpp)
target_link_libraries(schemagen PRIVATE graphqlpeg)

set(BOOST_COMPONENTS program_options)
set(BOOST_LIBRARIES Boost::program_options)

if(NOT MSVC)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem)
  set(BOOST_LIBRARIES ${BOOST_LIBRARIES} Boost::filesystem)
  target_compile_options(schemagen PRIVATE -DUSE_BOOST_FILESYSTEM)
endif()

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
target_link_libraries(schemagen PRIVATE ${BOOST_LIBRARIES})

add_custom_command(
  OUTPUT
    ${CMAKE_BINARY_DIR}/IntrospectionSchema.cpp
    ${CMAKE_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
  COMMAND schemagen --introspection
  DEPENDS schemagen
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Generating IntrospectionSchema files"
)

# graphqlservice
add_library(graphqlservice
  $<TARGET_OBJECTS:graphqlresponse>
  GraphQLService.cpp
  Introspection.cpp
  ${CMAKE_BINARY_DIR}/IntrospectionSchema.cpp)
target_link_libraries(graphqlservice PUBLIC
  graphqlpeg
  Threads::Threads)
target_include_directories(graphqlservice PRIVATE
  ${CMAKE_BINARY_DIR}/include)

# RapidJSON is the only option for JSON serialization used in this project, but if you want
# to use another JSON library you can implement an alternate version of the functions in
# JSONResponse.cpp to serialize to and from GraphQLResponse and build graphqljson from that.
# You will also need to define how to build the graphqljson library target with your
# implementation, and you should set BUILD_GRAPHQLJSON so that the test dependencies know
# about your version of graphqljson.
option(GRAPHQL_USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)

if(GRAPHQL_USE_RAPIDJSON)
  find_package(RapidJSON CONFIG REQUIRED)

  set(BUILD_GRAPHQLJSON ON)
  add_library(graphqljson JSONResponse.cpp)
  target_include_directories(graphqljson SYSTEM PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
endif()

# graphqljson
if(BUILD_GRAPHQLJSON)
  option(GRAPHQL_BUILD_TESTS "Build the tests and sample schema library." ON)

  target_link_libraries(graphqljson PUBLIC graphqlservice)

  install(TARGETS graphqljson
    EXPORT cppgraphqlgen-targets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
  install(FILES ${CMAKE_SOURCE_DIR}/include/graphqlservice/JSONResponse.h
    DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice
    CONFIGURATIONS Release)
else()
  set(GRAPHQL_BUILD_TESTS OFF CACHE BOOL "GRAPHQL_BUILD_TESTS depends on BUILD_GRAPHQLJSON" FORCE)
endif()

if(GRAPHQL_UPDATE_SAMPLES)
  install(FILES
      ${CMAKE_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
      ${CMAKE_BINARY_DIR}/IntrospectionSchema.cpp
    DESTINATION ${CMAKE_SOURCE_DIR}/samples/introspection)
endif()

install(TARGETS
    graphqlpeg
    graphqlservice
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(TARGETS schemagen
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION ${GRAPHQL_INSTALL_TOOLS_DIR}/${PROJECT_NAME}
  CONFIGURATIONS Release)

install(FILES
    ${CMAKE_SOURCE_DIR}/include/graphqlservice/GraphQLTree.h
    ${CMAKE_SOURCE_DIR}/include/graphqlservice/GraphQLResponse.h
    ${CMAKE_SOURCE_DIR}/include/graphqlservice/GraphQLService.h
    ${CMAKE_SOURCE_DIR}/include/graphqlservice/Introspection.h
    ${CMAKE_BINARY_DIR}/include/graphqlservice/IntrospectionSchema.h
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice
  CONFIGURATIONS Release)

# Enable version checks in find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(cmake/${PROJECT_NAME}-config-version.cmake COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})
