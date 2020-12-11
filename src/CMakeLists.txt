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
add_library(cppgraphqlgen::graphqlpeg ALIAS graphqlpeg)
target_link_libraries(graphqlpeg PUBLIC taocpp::pegtl)
target_include_directories(graphqlpeg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)
add_bigobj_flag(graphqlpeg)

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlpeg
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLPEG_DLL)
endif()

# graphqlresponse
add_library(graphqlresponse GraphQLResponse.cpp)
add_library(cppgraphqlgen::graphqlresponse ALIAS graphqlresponse)
target_include_directories(graphqlresponse PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:${GRAPHQL_INSTALL_INCLUDE_DIR}>)

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlresponse
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLRESPONSE_DLL)
endif()

# schemagen
if(GRAPHQL_BUILD_SCHEMAGEN)
  add_executable(schemagen SchemaGenerator.cpp)
  add_executable(cppgraphqlgen::schemagen ALIAS schemagen)
  target_link_libraries(schemagen PRIVATE
    graphqlpeg
    graphqlresponse)
  add_bigobj_flag(schemagen)
  
  set(BOOST_COMPONENTS program_options)
  set(BOOST_LIBRARIES Boost::program_options)

  # Try compiling a test program with std::filesystem or one of its alternatives.
  function(check_filesystem_impl FILESYSTEM_HEADER FILESYSTEM_NAMESPACE OPTIONAL_LIBS OUT_RESULT)
    set(TEST_FILE "test_${OUT_RESULT}.cpp")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/test_filesystem.cpp.in ${TEST_FILE} @ONLY)

    try_compile(TEST_RESULT
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
      CXX_STANDARD 17)

    if(NOT TEST_RESULT)
      # Retry with each of the optional libraries.
      foreach(OPTIONAL_LIB IN LISTS OPTIONAL_LIBS)
        try_compile(TEST_RESULT
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/${TEST_FILE}
          LINK_LIBRARIES ${OPTIONAL_LIB}
          CXX_STANDARD 17)

        if(TEST_RESULT)
          # Looks like the optional library was required, go ahead and add it to the link options.
          target_link_libraries(schemagen PRIVATE ${OPTIONAL_LIB})
          break()
        endif()
      endforeach(OPTIONAL_LIB)
    endif()

    set(${OUT_RESULT} ${TEST_RESULT} PARENT_SCOPE)
  endfunction(check_filesystem_impl)

  # Try compiling a minimal program with each header/namespace, in order of preference:
  #   C++17: #include <filesystem> // std::filesystem
  #   Experimental C++17: #include <experimental/filesystem> // std::experimental::filesystem
  #   Boost.Filesystem: #include <boost/filesystem.hpp> // boost::filesystem
  check_filesystem_impl("filesystem" "std::filesystem" "stdc++fs;c++fs" STD_FILESYTEM)
  if(STD_FILESYTEM)
    target_compile_definitions(schemagen PRIVATE USE_STD_FILESYSTEM)
  else()
    check_filesystem_impl("experimental/filesystem" "std::experimental::filesystem" "stdc++fs;c++fs" STD_EXPERIMENTAL_FILESYTEM)
    if(STD_EXPERIMENTAL_FILESYTEM)
      target_compile_definitions(schemagen PRIVATE USE_STD_EXPERIMENTAL_FILESYSTEM)
    else()
      set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem)
      set(BOOST_LIBRARIES ${BOOST_LIBRARIES} Boost::filesystem)
      target_compile_definitions(schemagen PRIVATE USE_BOOST_FILESYSTEM)
    endif()
  endif()

  find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
  target_link_libraries(schemagen PRIVATE ${BOOST_LIBRARIES})
  
  install(TARGETS schemagen
    EXPORT cppgraphqlgen-targets
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    RUNTIME DESTINATION ${GRAPHQL_INSTALL_TOOLS_DIR}/${PROJECT_NAME})
endif()

# introspection
if(GRAPHQL_UPDATE_SAMPLES)
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/../IntrospectionSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/../include/graphqlservice/IntrospectionSchema.h
    COMMAND ${CMAKE_COMMAND} -E make_directory include/graphqlservice
    COMMAND schemagen --introspection
    DEPENDS schemagen
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    COMMENT "Generating IntrospectionSchema files")

  file(GLOB OLD_INTROSPECTION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../samples/introspection/*)

  add_custom_command(
    OUTPUT updated_introspection
    COMMAND ${CMAKE_COMMAND} -E remove -f ${OLD_INTROSPECTION_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy ../IntrospectionSchema.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../samples/introspection/
    COMMAND ${CMAKE_COMMAND} -E copy ../include/graphqlservice/IntrospectionSchema.h ${CMAKE_CURRENT_SOURCE_DIR}/../samples/introspection/
    COMMAND ${CMAKE_COMMAND} -E touch updated_introspection
    DEPENDS 
      ${CMAKE_CURRENT_BINARY_DIR}/../IntrospectionSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/../include/graphqlservice/IntrospectionSchema.h
    COMMENT "Updating introspection files")

  add_custom_target(update_introspection ALL
    DEPENDS updated_introspection)
else()
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/../IntrospectionSchema.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/../include/graphqlservice/IntrospectionSchema.h
    COMMAND ${CMAKE_COMMAND} -E make_directory include/graphqlservice
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../samples/introspection/IntrospectionSchema.cpp .
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../samples/introspection/IntrospectionSchema.h include/graphqlservice
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    COMMENT "Copying IntrospectionSchema files")
endif()

# graphqlservice
add_library(graphqlservice
  GraphQLService.cpp
  GraphQLSchema.cpp
  Introspection.cpp
  Validation.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/../IntrospectionSchema.cpp)
add_library(cppgraphqlgen::graphqlservice ALIAS graphqlservice)
target_link_libraries(graphqlservice PUBLIC
    graphqlpeg
    Threads::Threads)
target_link_libraries(graphqlservice PUBLIC graphqlresponse)
target_include_directories(graphqlservice PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>)

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(graphqlservice
    PUBLIC GRAPHQL_DLLEXPORTS
    PRIVATE IMPL_GRAPHQLSERVICE_DLL)
endif()

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
  add_library(cppgraphqlgen::graphqljson ALIAS graphqljson)
  target_link_libraries(graphqljson PUBLIC graphqlresponse)
  target_include_directories(graphqljson SYSTEM PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

  if(WIN32 AND BUILD_SHARED_LIBS)
    target_compile_definitions(graphqljson
      PUBLIC GRAPHQL_DLLEXPORTS
      PRIVATE IMPL_JSONRESPONSE_DLL)
  endif()
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
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/JSONResponse.h
    CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
    DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice)
else()
  set(GRAPHQL_BUILD_TESTS OFF CACHE BOOL "GRAPHQL_BUILD_TESTS depends on BUILD_GRAPHQLJSON" FORCE)
endif()

install(TARGETS
    graphqlpeg
    graphqlresponse
    graphqlservice
  EXPORT cppgraphqlgen-targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLParse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLResponse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLSchema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLGrammar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/GraphQLTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/graphqlservice/Introspection.h
    ${CMAKE_CURRENT_BINARY_DIR}/../include/graphqlservice/IntrospectionSchema.h
  CONFIGURATIONS ${GRAPHQL_INSTALL_CONFIGURATIONS}
  DESTINATION ${GRAPHQL_INSTALL_INCLUDE_DIR}/graphqlservice)

install(EXPORT cppgraphqlgen-targets
  NAMESPACE cppgraphqlgen::
  DESTINATION ${GRAPHQL_INSTALL_CMAKE_DIR}/${PROJECT_NAME})
