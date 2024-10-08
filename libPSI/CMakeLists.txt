project(libPSI)

##############################################
##                 Link NTL                  #
##############################################
set(NTL_Lib_Dirs "${NTL_Dirs}src/")
set(NTL_Include_Dirs "${NTL_Dirs}include/")

find_library(NTL_LIB
  NAMES ntl libntl.a
  PATHS "${NTL_Lib_Dirs}"
  NO_DEFAULT_PATH
)

if(NOT NTL_LIB)
  if(EXISTS "${NTL_Lib_Dirs}libntl.a")
    set(NTL_LIB "${NTL_Lib_Dirs}libntl.a")
  else()
    message(FATAL_ERROR "Failed to find NTL at ${NTL_Lib_Dirs} or system locations.")
  endif()
endif()

message(STATUS "NTL  ${NTL_LIB}")
message(STATUS "NTL  ${NTL_Include_Dirs}")
message(STATUS "NTL  ${NTL_Lib_Dirs}")

##############################################
##                 Link GMP                  #
##############################################
set(GMP_Lib_Dirs "${GMP_Dirs}lib/")
set(GMP_Include_Dirs "${GMP_Dirs}include/")

find_library(
  GMP_LIB 
  NAMES gmp libgmp.a  
  PATHS "${GMP_Lib_Dirs}"
  NO_DEFAULT_PATH
)

if(NOT GMP_LIB)
  if(EXISTS "${GMP_Lib_Dirs}/libgmp.a")
    set(GMP_LIB "${GMP_Lib_Dirs}/libgmp.a")
  else()
    message(FATAL_ERROR "Failed to find GMP library at ${GMP_Lib_Dirs}.")
  endif()
endif()

message(STATUS "GMP  ${GMP_LIB}")
message(STATUS "GMP  ${GMP_Include_Dirs}")
message(STATUS "GMP  ${GMP_Lib_Dirs}")

##############################################
##                 Link GF2X                #
##############################################
set(GF2X_Lib_Dirs "${GF2X_Dirs}lib/")
set(GF2X_Include_Dirs "${GF2X_Dirs}include/")

find_library(
  GF2X_LIB 
  NAMES gf2x libgf2x.a  
  PATHS "${GF2X_Lib_Dirs}"
)

if(NOT GF2X_LIB)
  if(EXISTS "${GF2X_Lib_Dirs}libgf2x.a")
    set(GF2X_LIB "${GF2X_Lib_Dirs}libgf2x.a")
  else()
    message(FATAL_ERROR "Failed to find gf2x at ${GF2X_Lib_Dirs} or system locations.")
  endif()
endif()

message(STATUS "gf2x  ${GF2X_LIB}")
message(STATUS "gf2x ${GF2X_Include_Dirs}")
message(STATUS "gf2x ${GF2X_Lib_Dirs}")

##############################################
##               Link SODIUM                #
##############################################
set(SODIUM_Lib_Dirs "${SODIUM_Dirs}/src/libsodium/.libs/")
set(SODIUM_Include_Dirs "${SODIUM_Dirs}/src/libsodium/include/")

include_directories("${SODIUM_Include_Dirs}")

find_library(
  SODIUM_LIB
  NAMES libsodium.a
  PATHS "${SODIUM_Lib_Dirs}"
  NO_DEFAULT_PATH
)

if(NOT SODIUM_LIB)
  if(EXISTS "${SODIUM_Lib_Dirs}/libsodium.a")
    set(SODIUM_LIB "${SODIUM_Lib_Dirs}/libsodium.a")
  else()
    message(FATAL_ERROR "Failed to find libsodium at ${SODIUM_Lib_Dirs} or system locations.")
  endif()
endif()

message(STATUS "sodium library: ${SODIUM_LIB}")
message(STATUS "sodium include directory: ${SODIUM_Include_Dirs}")
message(STATUS "sodium lib directory: ${SODIUM_Lib_Dirs}")

#############################################
#                 Link Boost                #
#############################################

set(BOOST_ROOT "${libOTe_Dirs}cryptoTools/thirdparty/linux/boost/")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}stage/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}stage/lib")  

set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost 1.65.1 REQUIRED COMPONENTS system thread)

if (Boost_FOUND)
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
else()
    message(FATAL_ERROR "Failed to find Boost at ${BOOST_ROOT}")
endif()

message(STATUS "Boost library: ${Boost_LIBRARIES}")
message(STATUS "Boost include directory: ${BOOST_INCLUDEDIR}")
message(STATUS "Boost lib directory: ${BOOST_LIBRARYDIR}")

#############################################
#                 Link Miracl               #
#############################################
find_library(
  MIRACL_LIB 
  NAMES miracl  
  HINTS "${Miracl_Dirs}/miracl/source/"
)

if(NOT MIRACL_LIB)
  message(FATAL_ERROR "Failed to find Miracl at ${Miracl_Dirs}")
else()
  message(STATUS "Miracl at  ${MIRACL_LIB}")
endif()

#############################################
#                 Link libOTe               #
#############################################
find_library(
    libOTe_LIB
    NAMES libOTe
    HINTS "${libOTe_Dirs}/lib/")
	
find_library(
    cryptoTools_LIB
    NAMES cryptoTools
    HINTS "${libOTe_Dirs}/lib/")

#############################################
#              Build libPSI                 #
#############################################
file(GLOB_RECURSE SRCS *.cpp)

add_library(libPSI ${SRCS})

target_include_directories(libPSI PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(libPSI PUBLIC ${NTL_Include_Dirs})
target_include_directories(libPSI PUBLIC ${GMP_Include_Dirs})
target_include_directories(libPSI PUBLIC ${GF2X_Include_Dirs})
target_include_directories(libPSI PUBLIC ${SODIUM_Include_Dirs})
target_include_directories(libPSI PUBLIC ${CMAKE_SOURCE_DIR}/libPSI)

target_link_libraries(libPSI ${libOTe_LIB} ${cryptoTools_LIB} ${MIRACL_LIB} ${Boost_LIBRARIES} ${NTL_LIB} ${GMP_LIB} ${GF2X_LIB} ${SODIUM_LIB})
