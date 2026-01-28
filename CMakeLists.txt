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
    src/storage/record.cpp
    src/storage/table.cpp
    src/storage/slot_helpers.cpp
)

# B+ Tree sources
set(BTREE_SOURCES
    src/storage/btree/btree.cpp
    src/storage/btree/leaf.cpp
    src/storage/btree/internal.cpp
    src/storage/btree/helpers.cpp
)

# Create storage library (optional, for better organization)
# add_library(storage STATIC ${STORAGE_SOURCES})

# Test: Page validation
# add_executable(slot_validation 
#     tests/page/validation.cpp
#     ${STORAGE_SOURCES}
# )

# add_executable(test_validation 
#     tests/page/slot_validation.cpp
#     ${STORAGE_SOURCES}
# )

add_executable(test_validation
    tests/page/page_insert.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_page_allocation
    tests/page/page_allocation.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_btree
    tests/storage/btree_test/btree_test.cpp
    ${STORAGE_SOURCES}
    ${BTREE_SOURCES}
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

set_target_properties(test_page_allocation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(test_btree PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if (WIN32)
    target_link_options(test_page_allocation PRIVATE -mconsole)
    target_link_options(test_btree PRIVATE -mconsole)
endif()

# set_target_properties(test_page_insert PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
# )

# Optional: Create a custom target to run tests
add_custom_target(run_validation
    COMMAND test_validation
    DEPENDS test_validation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running validation test"
)

add_custom_target(run_page_insert
    COMMAND test_page_insert
    DEPENDS test_page_insert
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running page_insert test"
)

add_custom_target(run_page_allocation
    COMMAND test_page_allocation
    DEPENDS test_page_allocation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running page_allocation test"
)

add_custom_target(run_btree_test
    COMMAND test_btree
    DEPENDS test_btree
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running B+ tree test"
)
