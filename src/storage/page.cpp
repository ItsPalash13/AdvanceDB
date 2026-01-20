#include "storage/page.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <iostream>

inline PageHeader* get_header(Page& page) {
    return reinterpret_cast<PageHeader*>(page.data);
}

void init_page(Page& page, uint32_t page_id, PageType page_type, PageLevel page_level) {
    
    PageHeader* page_header = get_header(page);
    std::memset(page.data, 0, PAGE_SIZE);
    
    page_header->page_id = page_id;
    page_header->page_type = page_type;
    page_header->page_level= page_level;

    std::fill_n(page_header->reserved, sizeof(page_header->reserved) / sizeof(page_header->reserved[0]), 0);
    
    page_header->flags = 0;
    page_header->cell_count = 0;
    page_header->free_start = sizeof(PageHeader);
    page_header->free_end = PAGE_SIZE;
    page_header->parent_page_id = 0;
    page_header->lsn = 0;
}


// obtain a slot pointer
uint16_t* slot_ptr(Page& page, uint16_t index) {
    PageHeader* header = get_header(page);
    auto slot = page.data + header->free_end + (index * sizeof(uint16_t));
    return reinterpret_cast<uint16_t*>(slot);
}

void insert_slot(Page& page, uint16_t index, uint16_t record_offset) {
    PageHeader* header = get_header(page);
    
    uint16_t old_free_end = header->free_end;
    uint16_t current_count = header->cell_count;
    header->free_end -= sizeof(uint16_t);
    uint16_t new_free_end = header->free_end;

    // shift slots before insertion index to new positions
    for (uint16_t i = 0; i < index; ++i) {
        *reinterpret_cast<uint16_t*>(page.data + new_free_end + i * sizeof(uint16_t)) =
            *reinterpret_cast<uint16_t*>(page.data + old_free_end + i * sizeof(uint16_t));
    }
    
    // shift slots after insertion index to make room
    // iterate backwards safely without unsigned underflow
    for (uint16_t i = current_count; i > index; --i) {
        *reinterpret_cast<uint16_t*>(page.data + new_free_end + i * sizeof(uint16_t)) =
            *reinterpret_cast<uint16_t*>(page.data + old_free_end + (i - 1) * sizeof(uint16_t));
    }
    
    // write new slot
    *reinterpret_cast<uint16_t*>(page.data + new_free_end + index * sizeof(uint16_t)) = record_offset;
    header->cell_count += 1;
    assert(header->free_start <= header->free_end);
    assert(header->cell_count * sizeof(uint16_t) <= PAGE_SIZE);
}

void remove_slot(Page& page, uint16_t index) {
    PageHeader* header = get_header(page);

    if (index > header->cell_count - 1) {
        throw std::runtime_error("Could not remove an invalid slot");
    }
    
    uint16_t old_free_end = header->free_end;
    uint16_t current_count = header->cell_count;
    header->free_end += sizeof(uint16_t);
    uint16_t new_free_end = header->free_end;

    // shift slots before removed index to new positions
    for(uint16_t i = 0; i < static_cast<int>(index); ++i) {
        *reinterpret_cast<uint16_t*>(page.data + new_free_end + i * sizeof(uint16_t)) =
            *reinterpret_cast<uint16_t*>(page.data + old_free_end + i * sizeof(uint16_t));
    }
    
    // shift slots after removed index to fill gap
    for(uint16_t i = index + 1; i < current_count; ++i) {
        *reinterpret_cast<uint16_t*>(page.data + new_free_end + (i - 1) * sizeof(uint16_t)) =
            *reinterpret_cast<uint16_t*>(page.data + old_free_end + i * sizeof(uint16_t));
    }

    header->cell_count -= 1;
    assert(header->free_start <= header->free_end);
    assert(header->cell_count * sizeof(uint16_t) <= PAGE_SIZE);
}

