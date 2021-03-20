cmake_minimum_required(VERSION 3.12)
project(socklib)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(INCLUDE_DIR include) #Local include directory for headers

#Source files:
set(SOURCE_FILES socks.cpp)
set(HEADER_FILES "${INCLUDE_DIR}/socks.hpp")

#Main function: Providing a socket library
add_library(socks ${SOURCE_FILES})
target_include_directories(socks PUBLIC ${INCLUDE_DIR}) #Use the same includes for building as we do for using the built library

#Installing into appropriate locations
install(TARGETS socks)
install(FILES ${HEADER_FILES} DESTINATION include)

#Tests
option(DO_TESTS "Build and run associated tests" OFF)
if(DO_TESTS)
	add_executable(unit-test tests/unit.cpp)
	target_link_libraries(unit-test socks)
endif()