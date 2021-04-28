cmake_minimum_required(VERSION 3.12)
project(socklib)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(INCLUDE_DIR include) #Local include directory for headers

#Options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_STATIC_LIBS "Build static library" ON)
option(DO_TESTS "Build and run tests (Uses static linking)" OFF)

#Default build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

#Source files:
set(SOURCE_FILES socks.cpp)
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
	add_library(${LNAME} ${LCLASS} ${SOURCE_FILES})
	target_include_directories(${LNAME} PUBLIC ${INCLUDE_DIR}) #Use the same includes for building as we do for using the built library
	set_target_properties(${LNAME} PROPERTIES OUTPUT_NAME socks) #Same name for shared and static
	install(TARGETS ${LNAME})
endforeach()

#Installing into appropriate locations
install(FILES ${HEADER_FILES} DESTINATION include)

#Tests
if (DO_TESTS)
	add_executable(unit-test tests/unit.cpp)
	target_include_directories(unit-test PUBLIC ${INCLUDE_DIR})
	target_link_libraries(unit-test socks)
endif()