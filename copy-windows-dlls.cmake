# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

function(copy_windows_dlls CMAKE_BINARY_DIR)
  set(DLL_DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  file(GLOB IMPORTED_DLLS LIST_DIRECTORIES false
    "${CMAKE_BINARY_DIR}/*.dll"
    "${CMAKE_BINARY_DIR}/*.pdb"
	"${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_CONFIG_NAME}/*.dll"
    "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_CONFIG_NAME}/*.pdb")
  file(INSTALL ${IMPORTED_DLLS} DESTINATION ${DLL_DESTINATION})
endfunction(copy_windows_dlls)
