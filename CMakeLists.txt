cmake_minimum_required(VERSION 3.12)
project(socklib)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Source files:
set(SOURCE_FILES socks.cpp)

#Main function: Providing a socket library
add_library(socks ${SOURCE_FILES})
add_definitions( "-Wall -Werror -Wextra" )
add_definitions( "-E" )

#Tests
option(DO_TESTS "Build and run associated tests" OFF)
if(DO_TESTS)
	add_executable(unit_test ${SOURCE_FILES} tests/unit.cpp)
endif()