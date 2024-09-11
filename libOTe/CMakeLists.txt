cmake_minimum_required (VERSION 2.8)

project(libOTe)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

	############################################
	#          If top level cmake              #
	############################################

	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)

	############################################
	#           Flag and #defines              #
	############################################
	add_definitions(-DSOLUTION_DIR='${CMAKE_SOURCE_DIR}')
	set(CMAKE_C_FLAGS "-ffunction-sections -Wall  -maes -msse2 -msse4.1 -mpclmul -Wfatal-errors -pthread -Wno-strict-overflow  -fPIC -Wno-ignored-attributes")
	set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14")
	
	# Select flags.
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG") 
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -g -ggdb -rdynamic")
	SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -ggdb -rdynamic")
	
	############################################
	#           Build mode checks              #
	############################################
	
	# Set a default build type for single-configuration
	# CMake generators if no build type is set.
	if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	   SET(CMAKE_BUILD_TYPE Release)
	endif()

	if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
	  message(WARNING "\nConfiguration NOT set to Release. Please call cmake with -DCMAKE_BUILD_TYPE=Release  instead of ${CMAKE_BUILD_TYPE}")

		if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
		  #message(WARNING "Debug build.")
		elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release") 
		  #message(WARNING "Release build.")
		elseif("${CMAKE_BUILD_TYPE}" STREQUAL  "RELWITHDEBINFO")
		  #message(WARNING "RELWITHDEBINFO=Release with debug info build.")
		else()
		  message(ERROR ": Unknown build type - ${CMAKE_BUILD_TYPE}  Please use one of Debug, Release, or RELWITHDEBINFO\n\n" )
		endif()
	endif()
endif()

set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cryptoTools/thirdparty/linux/boost")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/stage/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost 1.65.1 REQUIRED COMPONENTS system thread)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Failed to find Boost at ${BOOST_ROOT}")
endif()
 

#############################################
#    Build cryptoTools (common utilities)   #
#############################################
#include_directories(cryptoTools)
add_subdirectory(cryptoTools)


#############################################
#               Build libOTe                #
#############################################
add_subdirectory(libOTe)
add_subdirectory(libOTe_Tests)

#############################################
#              Build Frontend               #
#############################################
add_subdirectory(frontend)

