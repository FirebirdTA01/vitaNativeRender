cmake_minimum_required(VERSION 3.13.4)

project(Vita_FinalRenderer)

option(BUILD_SHADERS "Build and link shader binaries" ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

# add_subdirectory(src)
# doing this will allow us to use the SHADER_OBJS variable in the src CMakeLists.txt file
add_subdirectory(src ${CMAKE_CURRENT_BINARY_DIR}/src)

if(BUILD_SHADERS)
    message(STATUS "Shaders will be built and linked")
else()
    message(STATUS "Skipping shader build and linkage")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring with DEBUG settings")
    # Debug-specific settings...
    target_compile_definitions(${PROJECT_NAME} PUBLIC _DEBUG_)
endif()
