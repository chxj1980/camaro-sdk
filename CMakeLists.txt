cmake_minimum_required (VERSION 2.8.12)

project (DeepCamera)

option (USE_NEON "Use ARM NEON" OFF)

aux_source_directory(${PROJECT_SOURCE_DIR}/Common/Source DIR_SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/Linux/Source DIR_LINUX_SRCS)

#Remove Pointgrey porting
list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/flycapturedevice.cpp)
list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/flycapturereader.cpp)
list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/pointgrey.cpp)

add_library(DeepCamera STATIC ${DIR_SRCS} ${DIR_LINUX_SRCS})

include_directories("${PROJECT_SOURCE_DIR}/Common/Include")
include_directories("${PROJECT_SOURCE_DIR}/Linux/Include")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if(USE_NEON)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard")
endif(USE_NEON)

#set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wno-strict-aliasing -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -Wno-strict-aliasing")
