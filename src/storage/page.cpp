#include "storage/page.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cassert>


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

    // sfhit all the slots until index to left
    for(int i = header->cell_count; i > index; --i) {
        *slot_ptr(page, i) = *slot_ptr(page, i - 1);
    }
    
    // write new slot 
    *slot_ptr(page, index) = record_offset;
    
    // inc cell count and shift free_end to left
    header->cell_count += 1;
    header->free_end -= sizeof(uint16_t);


    assert(header->free_start <= header->free_end);
    assert(header->cell_count * sizeof(uint16_t) <= PAGE_SIZE);

}

void remove_slot(Page& page, uint16_t index) {
    PageHeader* header = get_header(page);

    if (index > header->cell_count - 1) {
        throw std::runtime_error("Could not remove an invalid slot");
    }
    for(int i = index; i < header->cell_count - 1; ++i) {
        *slot_ptr(page, i) = *slot_ptr(page, i + 1);
    }

    header->cell_count -= 1;
    header->free_end += sizeof(uint16_t);

    assert(header->free_start <= header->free_end);
    assert(header->cell_count * sizeof(uint16_t) <= PAGE_SIZE);
}

