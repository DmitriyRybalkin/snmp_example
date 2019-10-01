cmake_minimum_required(VERSION 2.8)

PROJECT(PSKSnmp C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

message(WARNING "CMAKE_MODULE_PATH = " ${CMAKE_MODULE_PATH})

INSTALL(FILES cmake/Modules/FindNetSNMP.cmake DESTINATION share/cmake/Modules/)

add_subdirectory (src)