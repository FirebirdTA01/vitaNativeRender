#psvita-toolchain.cmake

if(DEFINED ENV{VITASDK})
	message(STATUS "VITASDK environment variable set")
	set(VITA_SDK $ENV{VITASDK})
	set(CMAKE_C_FLAGS -I${VITA_SDK}/include -I${VITA_SDK}/arm-vita-eabi/include)
	set(CMAKE_CXX_FLAGS -I${VITA_SDK}/include -I${VITA_SDK}/arm-vita-eabi/include -I${VITA_SDK}/arm-vita-eabi/include/c++/10.3.0)
	set(PSVITA_LIBS_PATH ${VITA_SDK}/arm-vita-eabi/lib)
else()
	message(FATAL_ERROR "Please define VITASDK environment variable!")
endif()

set(CMAKE_SYSTEM_NAME PSVITA)
set(CMAKE_SYSTEM_PROCESSOR arm)

#specify cross compilers and tools
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

#only use the psvita toolchain, no host tools
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# I just added this and it doesn't seem to make a difference
set(CMAKE_FIND_ROOT_PATH ${VITA_SDK}/arm-vita-eabi/include; ${VITA_SDK}/arm-vita-eabi/include/c++/10.3.0)

#set the compiler flags #maybe do this in another CMakeLists.txt depending on build type
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -fno-short-enums")

#specify the path to the ps vita libraries
set(PSVITA_LIBS_PATH ${VITA_SDK}/arm-vita-eabi/lib)