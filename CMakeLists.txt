cmake_minimum_required(VERSION 3.10)
project(AdvanceDB)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug symbols for all builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Storage library sources
set(STORAGE_SOURCES
    src/storage/disk_manager.cpp
    src/storage/page.cpp
)

# Create storage library (optional, for better organization)
# add_library(storage STATIC ${STORAGE_SOURCES})

# Test: Page validation
add_executable(slot_validation 
    tests/page/validation.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_validation 
    tests/page/slot_validation.cpp
    ${STORAGE_SOURCES}
)

# Optional: Parser executable
# add_executable(parser
#     src/parser/main.cpp
#     src/parser/parser.cpp
#     src/parser/statements.cpp
# )

# Output executables to a bin directory
set_target_properties(test_validation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Optional: Create a custom target to run tests
add_custom_target(run_validation
    COMMAND test_validation
    DEPENDS test_validation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running validation test"
)

