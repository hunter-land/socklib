cmake_minimum_required (VERSION 3.16.2)
project (tests)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIR "include") # Includes for testing
set(SOURCE_DIR "src") # Sources for testing

# Files
set(SOURCE_FILES "${SOURCE_DIR}/main.cpp" "${SOURCE_DIR}/steps.cpp" "${SOURCE_DIR}/utility.cpp")
set(HEADER_FILES "${INCLUDE_DIR}/steps.hpp" "${INCLUDE_DIR}/utility.hpp")

# Application
add_executable( tests ${SOURCE_FILES} )
target_include_directories( tests PRIVATE ${INCLUDE_DIR} )
# Targets for testing
set(TARGET_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include") # Include directory of target
find_library(SOCKLIB NAMES socks REQUIRED)
target_link_libraries(tests PRIVATE socks)
target_include_directories( tests PRIVATE ${TARGET_INCLUDE_DIR} )

# Threads
find_package(Threads REQUIRED)
target_link_libraries(tests PRIVATE Threads::Threads)
# Basic Test Framework
find_library(BTFLIB NAMES btf REQUIRED)
target_link_libraries(tests PRIVATE btf)
