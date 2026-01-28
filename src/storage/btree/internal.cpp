#include <cstdint>
#include <cassert>
#include "storage/page.hpp"
#include "storage/btree.hpp"
#include "storage/table_handle.hpp"
#include "storage/record.hpp"

uint32_t internal_find_child(Page& page, const Key& key) {
    PageHeader* ph = get_header(page);

    assert(ph->page_level == PageLevel::INTERNAL);

    int left = 0;
    int right = ph->cell_count - 1;
    int pos = ph->cell_count;

    while(left <= right) {
        int mid = (right + left) / 2;
        uint16_t mid_key_len;
        const uint8_t* mid_key = slot_key(page, mid, mid_key_len);

        auto cmp = compare_keys(key.data, key.size, mid_key, mid_key_len);

        if (cmp < 0) {
            pos = mid;
            right = mid - 1;
        } else {
            left = mid + 1;
        }

    }

    // In this B+ tree structure:
    // - Entries store keys with their RIGHT children (for keys >= that key)
    // - The leftmost child (for keys < first key) is stored in reserved[0-3] as uint32_t
    // - For root pages, root_page field also stores the leftmost child (for compatibility)
    
    // In this B+ tree structure:
    // - Entry[i] stores key[i] and child[i+1] (the RIGHT child of key[i], for keys >= key[i])
    // - The leftmost child[0] (for keys < key[0]) is stored in reserved field
    // - Binary search finds the first position where key >= entry[pos].key
    
    // If pos == 0, key < entry[0].key, so we need the leftmost child
    if (pos == 0) {
        uint32_t leftmost_child = *reinterpret_cast<uint32_t*>(ph->reserved);
        
        // Validate page ID (should be reasonable, not too large)
        if (leftmost_child != 0 && leftmost_child < 1000000 && leftmost_child != INVALID_PAGE_ID) {
            return leftmost_child;
        }
        // Fallback: for root pages, also check root_page field
        if (ph->parent_page_id == 0 && ph->root_page != 0 && ph->root_page != ph->page_id && ph->root_page < 1000000) {
            return ph->root_page;
        }
        // If no leftmost child stored, try to use entry[0]'s child as fallback
        // This shouldn't happen in a valid B+ tree, but might help debug
        if (ph->cell_count > 0) {
            InternalEntry* entry = reinterpret_cast<InternalEntry*>(page.data + *slot_ptr(page, 0));
            if (entry->child_page != 0 && entry->child_page < 1000000) {
                return entry->child_page;
            }
        }
        // Invalid state
        assert(false && "No valid leftmost child found");
        return 0;
    }
    
    // If pos == cell_count, key >= all keys, so we need the rightmost child
    // which is entry[cell_count-1].child_page
    if (pos == ph->cell_count) {
        if (ph->cell_count == 0) {
            return 0; // Empty internal node (shouldn't happen in valid B+ tree)
        }
        InternalEntry* last_entry = reinterpret_cast<InternalEntry*>(page.data + *slot_ptr(page, ph->cell_count - 1));
        return last_entry->child_page;
    }
    
    // For 0 < pos < cell_count:
    // - pos is the first position where key < entry[pos].key
    // - This means entry[pos-1].key <= key < entry[pos].key
    // - We need child[pos], which is entry[pos-1].child_page
    if (pos > 0 && pos <= ph->cell_count) {
        if (pos - 1 < ph->cell_count) {
            InternalEntry* entry = reinterpret_cast<InternalEntry*>(page.data + *slot_ptr(page, pos - 1));
            // Validate page ID
            if (entry->child_page == 0 || entry->child_page >= 1000000) {
                assert(false && "Invalid child page ID");
                return 0;
            }
            return entry->child_page;
        }
    }
    
    assert(false && "Invalid position in internal_find_child");
    return 0; // Should not reach here
}


uint16_t write_internal_entry(Page& page, const Key& key, uint32_t child) {
    PageHeader* ph = get_header(page);
    assert(ph->page_level== PageLevel::INTERNAL);

    uint32_t offset = ph->free_start;

    InternalEntry ieheader  {
        .key_size = key.size,
        .child_page = child
    };

    memcpy(page.data + offset, &ieheader, sizeof(ieheader));
    memcpy(page.data + offset + sizeof(ieheader), key.data, key.size);

    ph->free_start += sizeof(ieheader) + key.size;
    return offset;
}

bool insert_internal_no_split(Page& page, const Key& key, uint32_t child) {
    PageHeader* ph = get_header(page);
    assert(ph->page_level== PageLevel::INTERNAL);

    uint16_t rec_size = sizeof(InternalEntry) + key.size;
    if (!can_insert(page, rec_size)) return false;

    BSearchResult sr = search_record(page, key.data, key.size);

    if (sr.found) return false; // Key already exists, cannot insert duplicate
    
    uint16_t offset = write_internal_entry(page, key, child);
    insert_slot(page, sr.index, offset);
    return true;
}

SplitInternalResult split_internal_page(TableHandle& th, Page& page) {
    auto* ph = get_header(page);
    assert(ph->page_level == PageLevel::INTERNAL);

    uint32_t new_pid = allocate_page(th);

    Page new_page;
    init_page(new_page, new_pid, PageType::INDEX, PageLevel::INTERNAL);

    uint16_t total = ph->cell_count;
    if (total < 2) {
        assert(false && "Cannot split internal page with less than 2 elements");
        return {0, {nullptr, 0}};
    }
    uint16_t mid = total / 2;

    // Extract separator key BEFORE modifying the page (since we'll remove slots)
    uint16_t sep_len;
    const uint8_t* sep_data = slot_key(page, mid, sep_len);
    // Copy the separator key data to avoid invalid pointer after page modification
    uint8_t sep_buf[256]; // Max key size - adjust if needed
    if (sep_len > 256) {
        assert(false && "Key too large");
        return {0, {nullptr, 0}};
    }
    memcpy(sep_buf, sep_data, sep_len);
    Key sep { sep_buf, sep_len };

    // Copy entries from mid+1 to end to new page
    // The leftmost child of the new page is child[mid+1], which is entry[mid].child_page
    // (the right child of the separator key at entry[mid])
    uint32_t new_leftmost_child = 0;
    if (mid < total) {
        uint16_t mid_entry_offset = *slot_ptr(page, mid);
        auto* mid_entry = reinterpret_cast<InternalEntry*>(page.data + mid_entry_offset);
        new_leftmost_child = mid_entry->child_page; // This is child[mid+1]
    }
    
    for (uint16_t i = mid + 1; i < total; i++) {
        uint16_t offset = *slot_ptr(page, i);
        auto* ieentry = reinterpret_cast<InternalEntry*>(page.data + offset);

        uint16_t size = sizeof(InternalEntry) + ieentry->key_size;

        uint8_t buf[PAGE_SIZE];
        memcpy(buf, page.data + offset, size);

        uint16_t new_off = write_raw_record(new_page, buf, size);
        insert_slot(new_page, get_header(new_page)->cell_count, new_off);
        
        // Update parent_page_id of the child page that was moved to new page
        uint32_t child_page_id = ieentry->child_page;
        Page child_page;
        th.dm.read_page(child_page_id, child_page.data);
        get_header(child_page)->parent_page_id = new_pid;
        th.dm.write_page(child_page_id, child_page.data);
    }
    
    // Store the leftmost child of the new page in reserved field
    if (new_leftmost_child != 0) {
        *reinterpret_cast<uint32_t*>(get_header(new_page)->reserved) = new_leftmost_child;
    }

    // Remove slots from the end, working backwards
    // Note: cell_count decreases as we remove, so always remove the last slot
    uint16_t num_to_remove = total - mid;
    for(uint16_t j = 0; j < num_to_remove; j++) {
        // Always remove the last slot (which shifts indices)
        uint16_t last_index = get_header(page)->cell_count - 1;
        if (last_index >= mid) {
            remove_slot(page, last_index);
        } else {
            break; // Shouldn't happen, but safety check
        }
    }

    // Set parent_page_id for the new internal page
    auto* new_ph = get_header(new_page);
    new_ph->parent_page_id = ph->parent_page_id;

    th.dm.write_page(new_pid, new_page.data);

    return { new_pid, sep };
}

void create_new_root(TableHandle& th, uint32_t left, const Key& key, uint32_t right) {
    uint32_t new_root_id = allocate_page(th);

    Page root;
    init_page(root, new_root_id, PageType::INDEX, PageLevel::INTERNAL);

    // Store the leftmost child in the reserved field (as uint32_t)
    auto* root_ph = get_header(root);
    *reinterpret_cast<uint32_t*>(root_ph->reserved) = left;
    // Also store in root_page for root pages (for compatibility)
    root_ph->root_page = left;
    
    // Store the separator key with right child as the first entry
    uint32_t offset = write_internal_entry(root, key, right);
    insert_slot(root, 0, offset);

    th.root_page = new_root_id;

    // Update meta page
    Page meta;
    th.dm.read_page(0, meta.data);
    get_header(meta)->root_page = new_root_id;
    th.dm.write_page(0, meta.data);

    // Write the new root
    th.dm.write_page(new_root_id, root.data);

    // Update parent_page_id of both child pages
    Page left_page;
    th.dm.read_page(left, left_page.data);
    auto* left_ph = get_header(left_page);
    
    // Validate left page before modifying
    if (left_ph->page_id != left) {
        assert(false && "Left page ID mismatch");
    }
    
    left_ph->parent_page_id = new_root_id;
    th.dm.write_page(left, left_page.data);

    Page right_page;
    th.dm.read_page(right, right_page.data);
    auto* right_ph = get_header(right_page);
    
    right_ph->parent_page_id = new_root_id;
    th.dm.write_page(right, right_page.data);
}

void insert_into_parent(TableHandle& th, uint32_t left, const Key& key, uint32_t right) {
    Page left_page;
    th.dm.read_page(left, left_page.data);

    auto* lh = get_header(left_page);

    if (lh->parent_page_id == 0) {
        create_new_root(th, left, key, right);
        return;
    }

    uint32_t parent_pid = lh->parent_page_id;
    
    if (parent_pid == 0 || parent_pid == INVALID_PAGE_ID) {
        // This shouldn't happen if parent_page_id was set correctly
        create_new_root(th, left, key, right);
        return;
    }

    Page parent;
    th.dm.read_page(parent_pid, parent.data);
    
    auto* ph = get_header(parent);
    if (ph->page_level != PageLevel::INTERNAL) {
        // Parent page is corrupted or wrong type, create new root
        create_new_root(th, left, key, right);
        return;
    }

    // Check where to insert the key
    BSearchResult sr = search_record(parent, key.data, key.size);
    if (sr.found) {
        // Key already exists (shouldn't happen)
        create_new_root(th, left, key, right);
        return;
    }
    
    // When inserting after a split:
    // - 'left' is already a child of the parent (either leftmost or right child of some key)
    // - We're adding 'right' as a new child with separator 'key'
    // - If inserting at index 0, 'left' should be (or become) the leftmost child
    // - The new entry's right child is 'right'
    
    if (sr.index == 0) {
        // Inserting at the beginning - ensure 'left' is the leftmost child
        uint32_t current_leftmost = *reinterpret_cast<uint32_t*>(ph->reserved);
        // If current leftmost is not 'left', then 'left' was the right child of entry[0]
        // In that case, the old leftmost becomes the right child of the new entry
        // But actually, 'left' should already be a child, so if it's not leftmost,
        // it must be the right child of entry[0]. When we insert before entry[0],
        // 'left' becomes the new leftmost, and entry[0]'s right child becomes... wait,
        // entry[0]'s right child is already set, we're not changing it.
        // So I think we just need to set leftmost = left when inserting at 0
        *reinterpret_cast<uint32_t*>(ph->reserved) = left;
    }

    if (insert_internal_no_split(parent, key, right)) {
        th.dm.write_page(parent_pid, parent.data);
        return;
    }

    auto split = split_internal_page(th, parent);

    th.dm.write_page(parent_pid, parent.data);

    insert_into_parent(th, parent_pid, split.seperator_key, split.new_page);
}


