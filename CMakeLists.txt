cmake_minimum_required (VERSION 2.8.12)

project (DeepCamera)

include(FindCUDA)
include(CMakeDependentOption)

message("Processor Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message("OS Platform: ${CMAKE_SYSTEM}")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm[A-Za-z0-9]+$")
	option (USE_NEON "Use ARM NEON" ON )
	set(LIB_DEST arm)
	if(IS_DIRECTORY /lib/arm-linux-gnueabihf)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard")
	endif()
else()
	set(LIB_DEST ${CMAKE_SYSTEM_PROCESSOR})
endif()

aux_source_directory(${PROJECT_SOURCE_DIR}/Common/Source DIR_SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/Linux/Source DIR_LINUX_SRCS)

include_directories("${PROJECT_SOURCE_DIR}/Common/Include")
include_directories("${PROJECT_SOURCE_DIR}/Linux/Include")

OPTION( SUPPORT_POINTGREY "Support FlyCapture API for PointGrey Camera" OFF ) 

if (NOT SUPPORT_POINTGREY)
	list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/FlyCaptureDevice.cpp)
	list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/FlyCaptureReader.cpp)
	list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/PointGrey.cpp)
endif (NOT SUPPORT_POINTGREY)

OPTION( SUPPORT_JPEG_SEQUENCE "Support JPEG Sequence as Virtual Camera (Need turbojpeg Library)" OFF ) 

if (NOT SUPPORT_JPEG_SEQUENCE)
	list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/FileSource.cpp)
	list(REMOVE_ITEM DIR_SRCS ${PROJECT_SOURCE_DIR}/Common/Source/ImageSequence.cpp)
endif(NOT SUPPORT_JPEG_SEQUENCE)

add_library(${PROJECT_NAME} STATIC ${DIR_SRCS} ${DIR_LINUX_SRCS})

if (SUPPORT_JPEG_SEQUENCE)
	target_link_libraries(${PROJECT_NAME} turbojpeg)
endif (SUPPORT_JPEG_SEQUENCE)


OPTION( USE_V4L2_USER_POINTER "Use V4L2 User Pointer Buffer Mode" OFF )

if(CUDA_FOUND)
	CMAKE_DEPENDENT_OPTION(USE_CUDA "Enable CUDA Zero-Copy (User Pointer Mode Needed)" ON "USE_V4L2_USER_POINTER" OFF)
	if(USE_CUDA)
		include_directories("${CUDA_INCLUDE_DIRS}")
	endif(USE_CUDA)
endif(CUDA_FOUND)

SET(VIDEO_BUFFER_NUM 8 CACHE STRING "Video Buffer Number (Default 8)")

CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/sys_config.h.in ${PROJECT_SOURCE_DIR}/Common/Include/sys_config.h )

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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
endif(USE_NEON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wno-strict-aliasing -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -Wno-strict-aliasing")

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/lib" CACHE PATH "Directory for install" FORCE)

install(TARGETS ${PROJECT_NAME} DESTINATION ${LIB_DEST})

message("Build configuration: ${CMAKE_BUILD_TYPE}")
message("Done.")
