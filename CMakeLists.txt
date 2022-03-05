cmake_minimum_required(VERSION 3.12)
project(socks VERSION 0.9.0 DESCRIPTION "A C++ wrapping of C BSD sockets")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include") #Local include directory for headers
include(GNUInstallDirs) #GNUInstallDirs (a default module) is required for this CMake

#Options
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

#Default build type
if (NOT CMAKE_BUILD_TYPE) #If the user hasn't set it, we set it
	set(CMAKE_BUILD_TYPE Release)
endif()

#Source files:
set(SOURCE_FILES "socks.cpp" "addrs.cpp" "errors.cpp")
set(HEADER_FILES "${INCLUDE_DIR}/socks.hpp" "${INCLUDE_DIR}/addrs.hpp" "${INCLUDE_DIR}/errors.hpp")

add_library(socks ${SOURCE_FILES})
# Set target properties for version and the related (public) headers
set_target_properties(socks PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(socks PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
# Declare target as something to be installed with the install target
install(TARGETS socks
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/socks")
