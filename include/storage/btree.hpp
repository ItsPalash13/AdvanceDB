#pragma once
#include <cstdint>
#include "storage/table_handle.hpp"
#include "storage/page.hpp"
#include <vector>
struct Key {
    const uint8_t* data;
    uint16_t size;
};

using Value = Key;

struct SplitLeafResult {
    uint32_t new_page;
    Key seperator_key;
};

using SplitInternalResult = SplitLeafResult;

// Main B+ tree operations
bool btree_search(TableHandle& th, const Key& key, Value& value);
bool btree_insert(TableHandle& th, const Key& key, const Value& value);

#pragma pack(push, 1)
struct InternalEntry {
    uint16_t key_size;
    uint32_t child_page;
    uint8_t key[];
};
#pragma pack(pop)


//helpers 
uint16_t write_raw_record(Page& page, const uint8_t* raw, uint16_t size);

// leaf 
uint32_t find_leaf_page(TableHandle& th, const Key& key, Page& out_page);
bool btree_insert_leaf_no_split(TableHandle& th, uint32_t page_id, Page& page, const Key& key, const Value& value);
SplitLeafResult split_leaf_page(TableHandle& th, Page& page);

// internal
uint32_t internal_find_child(Page& page, const Key& key);
bool insert_internal_no_split(Page& page, const Key& key, uint32_t child);
SplitInternalResult split_internal_page(TableHandle& th, Page& page);
void create_new_root(TableHandle& th, uint32_t left, const Key& key, uint32_t right);
void insert_into_parent(TableHandle& th, uint32_t left, const Key& key, uint32_t right);