cmake_minimum_required(VERSION 3.12)
project(socklib VERSION 0.8.0 DESCRIPTION "A C++ wrapping of C BSD sockets")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include") #Local include directory for headers

#Options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_STATIC_LIBS "Build static library" ON)
option(DO_TESTS "Build and run tests" OFF)

#Default build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

#Source files:
set(SOURCE_FILES "socks.cpp")
set(HEADER_FILES "${INCLUDE_DIR}/socks.hpp")

#Building and installing libraries (shared and/or static)
#Setup of library class targets
set(LIBRARY_CLASSES STATIC)
if (BUILD_SHARED_LIBS)
	list(APPEND LIBRARY_CLASSES SHARED)
endif()
if (NOT BUILD_STATIC_LIBS)
	list(REMOVE_ITEM LIBRARY_CLASSES STATIC)
endif()
#Build each library class
foreach (LCLASS IN LISTS LIBRARY_CLASSES)
	set(LNAME socks_${LCLASS})
	# Add this library as a target
	add_library(${LNAME} ${LCLASS} ${SOURCE_FILES})
	# Library includes files from its own include dir
	target_include_directories(${LNAME} PRIVATE ${INCLUDE_DIR})
	# Set target properties for output name, version, and the related (public) headers
	set_target_properties(${LNAME} PROPERTIES OUTPUT_NAME socks) #Same name for shared and static
	set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION})
	set_target_properties(${LNAME} PROPERTIES PUBLIC_HEADER ${HEADER_FILES})
	# Declare target as something to be installed with the install target
	install(TARGETS ${LNAME}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
	
	#Installing into appropriate locations
	#install(FILES ${HEADER_FILES} DESTINATION include)
endforeach()


#Tests
if (DO_TESTS)
	add_executable(unit-test tests/unit.cpp)
	#Link library
	find_library(SOCKLIB NAMES socks)
	target_link_libraries(unit-test PRIVATE ${SOCKLIB})
endif()