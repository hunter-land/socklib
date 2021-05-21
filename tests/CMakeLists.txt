cmake_minimum_required(VERSION 3.12)
project(socklib-tests VERSION 0.8.0 DESCRIPTION "Tests for the SocKetS library")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)



# unit-test is the target for unit testing
add_executable(unit-test unit.cpp)
# Link socks library
find_library(SOCKLIB NAMES socks)
target_link_libraries(unit-test PRIVATE socks)
# Unit test plays both sides, it needs threads
find_package(Threads REQUIRED)
target_link_libraries(unit-test PRIVATE Threads::Threads)
