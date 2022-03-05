cmake_minimum_required(VERSION 3.16.3)
project(echo_serv VERSION 0.8.0 DESCRIPTION "Echo Serv Example for the SocKetS library")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include(GNUInstallDirs) #GNUInstallDirs (a default module) is required for this CMake

# test is the target for unit testing
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
# Link socks library
find_library(socks NAMES socks HINTS "${CMAKE_INSTALL_LIBDIR}" REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE socks)