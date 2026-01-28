#include <cstdint>
#include "storage/page.hpp"
#include "storage/btree.hpp"
#include "storage/table_handle.hpp"
#include "storage/record.hpp"
#include <assert.h>

uint32_t find_leaf_page(TableHandle& th, const Key& key, Page& out_page) {
    uint32_t page_id = th.root_page;
    int depth = 0;
    
    while(1) {
        th.dm.read_page(page_id, out_page.data);
        
        auto* ph = get_header(out_page);
        
        if (ph->page_level == PageLevel::LEAF) {
            return page_id;
        }
        
        uint32_t next_page_id = internal_find_child(out_page, key);
        
        if (next_page_id == 0 || next_page_id >= 1000000) {
            return UINT32_MAX;
        }
        
        page_id = next_page_id;
        depth++;
        
        if (depth > 100) {
            return UINT32_MAX;
        }
    }

    return UINT32_MAX; // for any error
}


bool btree_insert_leaf_no_split(TableHandle& th, uint32_t page_id, Page& page, const Key& key, const Value& value) {
    uint16_t rec_size = record_size(key.size, value.size);
    if (!can_insert(page, rec_size)) {
        return false;
    }

    page_insert(page, key.data, key.size, value.data, value.size);
    
    // Validate page header before writing
    auto* ph_after = get_header(page);
    if (ph_after->free_start < sizeof(PageHeader) || ph_after->free_start >= PAGE_SIZE ||
        ph_after->free_end < sizeof(PageHeader) || ph_after->free_end > PAGE_SIZE ||
        ph_after->free_start > ph_after->free_end) {
        assert(false && "Corrupted page header");
        return false;
    }

    th.dm.write_page(page_id, page.data);
    return true;
}


SplitLeafResult split_leaf_page(TableHandle& th, Page& page) {
    PageHeader* ph = get_header(page);
    assert(ph->page_level == PageLevel::LEAF);
    
    // Validate page header - if corrupted, we can't split
    if (ph->free_start < sizeof(PageHeader) || ph->free_start >= PAGE_SIZE ||
        ph->free_end < sizeof(PageHeader) || ph->free_end > PAGE_SIZE ||
        ph->free_start > ph->free_end) {
        assert(false && "Cannot split corrupted page");
        return {0, {nullptr, 0}};
    }

    uint32_t new_page_id = allocate_page(th);

    Page new_page;
    init_page(new_page, new_page_id, PageType::DATA, PageLevel::LEAF);
    
    // Set parent page ID for the new page
    PageHeader* new_ph = get_header(new_page);
    new_ph->parent_page_id = ph->parent_page_id;

    uint16_t total = ph->cell_count;
    
    if (total < 2) {
        assert(false && "Cannot split page with less than 2 elements");
        return {0, {nullptr, 0}};
    }
    uint16_t split_index = total / 2;
    // Ensure at least one element stays in left page
    if (split_index == 0) {
        split_index = 1;
    }

    // Copy records from split_index to end to new page
    for(uint16_t i = split_index; i < total; i++) {
        uint16_t offset = *slot_ptr(page, i);

        auto* rh = reinterpret_cast<RecordHeader*>(page.data + offset);
        auto rec_size = record_size(rh->key_size, rh->value_size);

        uint8_t buffer[PAGE_SIZE];
        memcpy(buffer, page.data + offset, rec_size);

        uint16_t new_offset = write_raw_record(new_page, buffer, rec_size);
        insert_slot(new_page, get_header(new_page)->cell_count, new_offset);
    }

    // Remove slots from the end, working backwards
    // Note: cell_count decreases as we remove, so we need to be careful
    // We remove from total-1 down to split_index
    uint16_t num_to_remove = total - split_index;
    for(uint16_t j = 0; j < num_to_remove; j++) {
        // Always remove the last slot (which shifts indices)
        uint16_t last_index = get_header(page)->cell_count - 1;
        if (last_index >= split_index) {
            remove_slot(page, last_index);
        } else {
            break; // Shouldn't happen, but safety check
        }
    }

    uint16_t sep_len;
    const uint8_t* sep_data;
    
    // Get separator key from the new page (first key in right page)
    // If new page is empty (can happen with single large record), use first key from left page
    if (new_ph->cell_count > 0) {
        sep_data = slot_key(new_page, 0, sep_len);
    } else {
        // New page is empty, use first key from left page as separator
        // This happens when splitting a single large record
        sep_data = slot_key(page, 0, sep_len);
    }

    // Copy the separator key data before writing the page (to avoid invalid pointer)
    uint8_t sep_buf[256]; // Max key size
    if (sep_len > 256) {
        assert(false && "Key too large");
        return {0, {nullptr, 0}};
    }
    memcpy(sep_buf, sep_data, sep_len);
    Key sep_key {sep_buf, sep_len};

    th.dm.write_page(new_page_id, new_page.data);

    return {
        new_page_id,
        sep_key
    };
}