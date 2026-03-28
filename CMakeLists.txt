cmake_minimum_required(VERSION 3.20)
project(MemoryManagmentTool VERSION 1.0 LANGUAGES CXX)

# Configuration for modern C++ 23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Automatic recursive file discovery for the library logic (excluding main.cpp)
file(GLOB_RECURSE LIB_SOURCES 
    "src/*.cpp" 
    "src/*.hpp" 
    "src/*.h"
)
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# Create an Object Library to share code between the App and Tests without recompiling
add_library(ProjectLogic OBJECT ${LIB_SOURCES})
set_target_properties(ProjectLogic PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ProjectLogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Create the main executable by linking the app's entry point with the logic library
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ProjectLogic)

# Enables the CTest framework and looks for GTest on the system
find_package(GTest REQUIRED)
enable_testing()

# Includes the 'tests' subfolder, which must contain its own CMakeLists.txt
add_subdirectory(tests)

message(STATUS "Build target: ${PROJECT_NAME}")