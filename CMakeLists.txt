cmake_minimum_required(VERSION 3.12)
project(socklib VERSION 0.8.0 DESCRIPTION "A C++ wrapping of C BSD sockets")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include") #Local include directory for headers

#Options

#Default build type
if (NOT CMAKE_BUILD_TYPE) #If the user hasn't set it, we set it
	set(CMAKE_BUILD_TYPE Release)
endif()

#Source files:
set(SOURCE_FILES "socks.cpp")
set(HEADER_FILES "${INCLUDE_DIR}/socks.hpp")

add_library(socks ${SOURCE_FILES})
# Set target properties for version and the related (public) headers
set_target_properties(socks PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(socks PROPERTIES PUBLIC_HEADER ${HEADER_FILES})
# Declare target as something to be installed with the install target
install(TARGETS ${LNAME}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
		#PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${HEADER_FILES} TYPE INCLUDE)
