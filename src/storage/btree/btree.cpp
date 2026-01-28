#include <cstdint>
#include "storage/page.hpp"
#include "storage/table_handle.hpp"
#include "storage/record.hpp"
#include "storage/btree.hpp"
#include <cstring>
#include <cassert>

extern uint32_t find_leaf_page(TableHandle& th, const Key& key, Page& out_page);
extern bool btree_insert_leaf_no_split(TableHandle& th, uint32_t page_id, Page& page, const Key& key, const Value& value);
extern SplitLeafResult split_leaf_page(TableHandle& th, Page& page);
extern void insert_into_parent(TableHandle& th, uint32_t left, const Key& key, uint32_t right);
extern uint16_t write_raw_record(Page& page, const uint8_t* raw, uint16_t size);

bool btree_search(TableHandle& th, const Key& key, Value& value) {
    if (th.root_page == 0) {
        return false; // Empty tree
    }

    Page leaf_page;
    uint32_t leaf_page_id = find_leaf_page(th, key, leaf_page);
    
    BSearchResult result = search_record(leaf_page, key.data, key.size);
    
    if (!result.found) {
        return false; // Key not found
    }
    
    // Extract the value
    uint16_t value_len;
    const uint8_t* value_data = slot_value(leaf_page, result.index, value_len);
    
    // Set the value (using const_cast since Value = Key has const uint8_t* but we need to set it)
    // Note: The caller should copy the data if they need to persist it, as the page data
    // may be modified or evicted from memory.
    value.size = value_len;
    value.data = value_data; // Pointer to data in page buffer
    
    return true;
}

bool btree_insert(TableHandle& th, const Key& key, const Value& value) {
    // Handle empty tree - create root leaf page
    if (th.root_page == 0) {
        uint32_t root_page_id = allocate_page(th);
        Page root;
        init_page(root, root_page_id, PageType::DATA, PageLevel::LEAF);
        th.root_page = root_page_id;
        
        // Update meta page
        Page meta;
        th.dm.read_page(0, meta.data);
        get_header(meta)->root_page = root_page_id;
        th.dm.write_page(0, meta.data);
        
        // Insert first record
        page_insert(root, key.data, key.size, value.data, value.size);
        th.dm.write_page(root_page_id, root.data);
        return true;
    }
    
    // Find the leaf page where this key should be inserted
    Page leaf_page;
    uint32_t leaf_page_id = find_leaf_page(th, key, leaf_page);
    
    // Check if key already exists
    BSearchResult search_result = search_record(leaf_page, key.data, key.size);
    if (search_result.found) {
        return false; // Key already exists, cannot insert duplicate
    }
    
    // Try to insert without splitting (pass the already-read page)
    if (btree_insert_leaf_no_split(th, leaf_page_id, leaf_page, key, value)) {
        return true;
    }
    
    // Page is full, need to split
    // Note: leaf_page still contains the original data since btree_insert_leaf_no_split
    // returns false without modifying it when the page is full
    SplitLeafResult split_result = split_leaf_page(th, leaf_page);
    
    // Validate page header before writing
    auto* after_split_ph = get_header(leaf_page);
    if (after_split_ph->page_id != leaf_page_id) {
        assert(false && "Page ID mismatch after split");
    }
    
    // Write the left page back (it was modified by split_leaf_page)
    th.dm.write_page(leaf_page_id, leaf_page.data);
    
    // Determine which page to insert into (left or right)
    // Read the new page to get the separator key (since split_result.seperator_key.data 
    // points to local data in split_leaf_page which is invalid after return)
    Page new_page;
    th.dm.read_page(split_result.new_page, new_page.data);
    
    // Extract separator key from the new page (first key in right page)
    // If the new page is empty (can happen when splitting a single large record),
    // use the separator key from split_result instead
    Key sep_key;
    PageHeader* new_ph = get_header(new_page);
    if (new_ph->cell_count > 0) {
        // New page has records, extract separator key from it
        uint16_t sep_len;
        const uint8_t* sep_data = slot_key(new_page, 0, sep_len);
        
        uint8_t sep_buf[256]; // Max key size - adjust if needed
        if (sep_len > 256) {
            assert(false && "Key too large");
            return false;
        }
        memcpy(sep_buf, sep_data, sep_len);
        sep_key = {sep_buf, sep_len};
    } else {
        // New page is empty (single large record case), use separator key from split_result
        // Copy it to avoid invalid pointer
        uint8_t sep_buf[256];
        if (split_result.seperator_key.size > 256) {
            assert(false && "Key too large");
            return false;
        }
        memcpy(sep_buf, split_result.seperator_key.data, split_result.seperator_key.size);
        sep_key = {sep_buf, split_result.seperator_key.size};
    }
    
    int cmp = compare_keys(key.data, key.size, sep_key.data, sep_key.size);
    
    if (cmp < 0) {
        // Insert into left page (original page)
        if (!can_insert(leaf_page, record_size(key.size, value.size))) {
            // Left page doesn't have space - this can happen when splitting a single large record
            // In this case, move the large record from left to right, then insert into left
            if (new_ph->cell_count == 0 && after_split_ph->cell_count == 1) {
                // Move the large record from left to right page
                // Get the record offset first (before any modifications)
                uint16_t large_offset = *slot_ptr(leaf_page, 0);
                
                // Read the record header to get sizes
                RecordHeader* large_rh = reinterpret_cast<RecordHeader*>(leaf_page.data + large_offset);
                uint16_t large_key_len = large_rh->key_size;
                uint16_t large_value_len = large_rh->value_size;
                
                // Copy large record to buffer (including header)
                uint16_t large_rec_size = record_size(large_key_len, large_value_len);
                std::vector<uint8_t> large_rec_buf(large_rec_size);
                memcpy(large_rec_buf.data(), leaf_page.data + large_offset, large_rec_size);
                
                // Extract key data for separator (before removing slot)
                uint8_t large_key_buf[256];
                if (large_key_len > 256) {
                    assert(false && "Key too large");
                    return false;
                }
                memcpy(large_key_buf, large_rec_buf.data() + sizeof(RecordHeader), large_key_len);
                
                // Remove large record from left page FIRST (before writing to right page)
                remove_slot(leaf_page, 0);
                
                // Write large record to right page
                uint16_t new_offset = write_raw_record(new_page, large_rec_buf.data(), large_rec_size);
                insert_slot(new_page, 0, new_offset);
                
                // Write both pages
                th.dm.write_page(leaf_page_id, leaf_page.data);
                th.dm.write_page(split_result.new_page, new_page.data);
                
                // Re-read the left page to ensure we have the latest state
                th.dm.read_page(leaf_page_id, leaf_page.data);
                
                // Now insert the new record into left page
                if (!can_insert(leaf_page, record_size(key.size, value.size))) {
                    assert(false && "Left page still doesn't have space after moving large record");
                    return false;
                }
                page_insert(leaf_page, key.data, key.size, value.data, value.size);
                th.dm.write_page(leaf_page_id, leaf_page.data);
                
                // Update separator key to be the large record's key (first key in right page)
                Key new_sep_key = {large_key_buf, large_key_len};
                
                // Update parent with the correct separator key
                insert_into_parent(th, leaf_page_id, new_sep_key, split_result.new_page);
                return true;
            } else {
                assert(false && "Left page doesn't have space after split");
                return false;
            }
        } else {
            page_insert(leaf_page, key.data, key.size, value.data, value.size);
            th.dm.write_page(leaf_page_id, leaf_page.data);
        }
    } else {
        // Insert into right page (new page)
        if (!can_insert(new_page, record_size(key.size, value.size))) {
            assert(false && "Right page doesn't have space after split");
            return false;
        }
        page_insert(new_page, key.data, key.size, value.data, value.size);
        th.dm.write_page(split_result.new_page, new_page.data);
    }
    
    // Update parent to include the new separator key (use sep_key which points to valid data)
    insert_into_parent(th, leaf_page_id, sep_key, split_result.new_page);
    
    return true;
}
