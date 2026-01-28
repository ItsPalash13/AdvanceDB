#!/bin/bash
# Compile and run B+ tree test

g++ -std=c++17 -I./include \
    tests/storage/btree_test/btree_test.cpp \
    src/storage/disk_manager.cpp \
    src/storage/page.cpp \
    src/storage/record.cpp \
    src/storage/table.cpp \
    src/storage/slot_helpers.cpp \
    src/storage/btree/btree.cpp \
    src/storage/btree/leaf.cpp \
    src/storage/btree/internal.cpp \
    src/storage/btree/helpers.cpp \
    -o test_btree.exe

# Run the test
./test_btree.exe
