cmake_minimum_required(VERSION 3.12)
project(socks VERSION 0.9.0 DESCRIPTION "A C++ wrapping of C BSD sockets")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_RELEASE "-Os ${CMAKE_CXX_FLAGS_RELEASE}") #Release build should include -Os by default

# Set our public include directory for our headers
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
include(GNUInstallDirs) #GNUInstallDirs (a default module) is required for this CMake

# Define options and defaults
option(BUILD_SHARED_LIBS "Build as a shared library" ON)
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release) #Build release or debug library
endif()
option(BUILD_TESTS "Build tests for library" OFF)
# Other flags
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Set file variables
set(SOURCE_FILES "${SOURCE_DIR}/socks.cpp" "${SOURCE_DIR}/addrs.cpp" "${SOURCE_DIR}/errors.cpp" "${SOURCE_DIR}/initialization.cpp")
set(HEADER_FILES "${INCLUDE_DIR}/socks.hpp" "${INCLUDE_DIR}/addrs.hpp" "${INCLUDE_DIR}/errors.hpp" "${INCLUDE_DIR}/initialization.hpp" "${INCLUDE_DIR}/macros.hpp")

# Define library and properties
add_library(socks ${SOURCE_FILES})
target_compile_definitions(socks PRIVATE IS_SKS_SOURCE)
set_target_properties(socks PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(socks PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
target_include_directories(socks PRIVATE ${INCLUDE_DIR})

# Windows needs WinSock linked
if(WIN32)
  target_link_libraries(socks wsock32 ws2_32)
endif()

# Define install options
install(
	TARGETS socks
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/socks"
)

# Tests
if (${BUILD_TESTS})
	add_subdirectory(tests)
endif()
