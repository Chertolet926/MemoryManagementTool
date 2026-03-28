# Automatically find all .cpp files in the current directory
# CONFIGURE_DEPENDS ensures CMake re-runs if a file is added/removed
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Create the test executable using discovered test files
add_executable(unit_tests ${TEST_SOURCES})

# Link GoogleTest and the project's logic library
# GTest::gtest_main provides the main() function automatically
target_link_libraries(unit_tests 
    PRIVATE 
    GTest::gtest_main 
    ProjectLogic
)

# Handle include paths for the test runner to find project headers
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Discover individual tests for CTest integration
include(GoogleTest)
gtest_discover_tests(unit_tests)
