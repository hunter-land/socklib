cmake_minimum_required(VERSION 3.18)
project(socklib-tests VERSION 0.8.0 DESCRIPTION "Tests for the SocKetS library")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include(GNUInstallDirs) #GNUInstallDirs (a default module) is required for this CMake



# test is the target for unit testing
add_executable(tests driver.cpp testing.cpp steps.cpp utility.cpp)
# Link socks library
find_library(socks NAMES socks HINTS "${CMAKE_INSTALL_LIBDIR}" REQUIRED)
target_link_libraries(tests PRIVATE socks)
# Test play both sides, it needs threads
find_package(Threads REQUIRED)
target_link_libraries(tests PRIVATE Threads::Threads)
