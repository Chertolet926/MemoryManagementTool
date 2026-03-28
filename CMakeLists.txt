cmake_minimum_required(VERSION 3.20)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS 
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

project(MemoryManagementTool 
    VERSION 1.0.0
    DESCRIPTION "Library for parsing Linux memory management files"
    LANGUAGES CXX
)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position-independent code for tests
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Automatic recursive file discovery for the library logic (excluding main.cpp)
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.hpp"
    "src/*.h"
)

list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# Create an Object Library to share code between the App and Tests without recompiling
add_library(ProjectLogic OBJECT ${LIB_SOURCES})
set_target_properties(ProjectLogic PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_VISIBILITY_PRESET hidden
)
target_include_directories(ProjectLogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Compiler warnings (cross-platform)
target_compile_options(ProjectLogic PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
        -Wcast-align -Wunused -Wnull-dereference -Wdouble-promotion
        -Wformat=2 -Wimplicit-fallthrough
        # Suppress false-positive maybe-uninitialized warning from Boost.Spirit X3
        -Wno-maybe-uninitialized
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive- /w14640 /w14242
    >
)

# Debug/Release options
target_compile_options(ProjectLogic PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-O0 -g3>
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O3 -DNDEBUG>
)

# Create the main executable by linking the app's entry point with the logic library
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ProjectLogic)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)

# Enables the CTest framework and looks for GTest on the system
find_package(GTest REQUIRED)
enable_testing()

# Includes the 'tests' subfolder, which must contain its own CMakeLists.txt
add_subdirectory(tests)

# Build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build target: ${PROJECT_NAME} v${PROJECT_VERSION}")