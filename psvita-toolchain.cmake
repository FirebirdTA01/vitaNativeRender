# psvita-toolchain.cmake

if(DEFINED ENV{VITASDK})
    message(STATUS "VITASDK environment variable set")
    set(VITA_SDK $ENV{VITASDK})
else()
    message(FATAL_ERROR "Please define VITASDK environment variable!")
endif()

# Ensure CMake can find the PSVITA platform definition
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${VITA_SDK}/include -I${VITA_SDK}/arm-vita-eabi/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${VITA_SDK}/include -I${VITA_SDK}/arm-vita-eabi/include -I${VITA_SDK}/arm-vita-eabi/include/c++/10.3.0")

set(CMAKE_SYSTEM_NAME PSVITA)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify cross compilers and tools
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

# Only use the PSVita toolchain, no host tools
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_ROOT_PATH
    ${VITA_SDK}/arm-vita-eabi/include
    ${VITA_SDK}/arm-vita-eabi/include/c++/10.3.0
)

# Specify the path to the PS Vita libraries
set(PSVITA_LIBS_PATH ${VITA_SDK}/arm-vita-eabi/lib)

