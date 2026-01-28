#include <storage/page.hpp>
#include <storage/record.hpp>
#include <stdexcept>
#include <cassert>


// obtain a slot pointer
uint16_t* slot_ptr(Page& page, uint16_t index) {
    PageHeader* header = get_header(page);
    auto slot = page.data + header->free_end + (index * sizeof(uint16_t));
    return reinterpret_cast<uint16_t*>(slot);
}

const uint8_t* slot_key(Page& page, uint16_t slot_index, uint16_t& key_len) {
    PageHeader* header = get_header(page);
    
    // Validate slot_index
    if (slot_index >= header->cell_count) {
        key_len = 0;
        return nullptr;
    }
    
    uint16_t record_offset = *slot_ptr(page, slot_index);
    
    // Validate record_offset
    if (record_offset < sizeof(PageHeader) || record_offset >= PAGE_SIZE) {
        // Invalid offset - return empty key to avoid corruption
        key_len = 0;
        return nullptr;
    }
    
    RecordHeader* record_header = reinterpret_cast<RecordHeader*>(page.data + record_offset);
    
    // Validate record header
    if (record_header->key_size > PAGE_SIZE || record_header->value_size > PAGE_SIZE) {
        key_len = 0;
        return nullptr;
    }

    key_len = record_header->key_size;
    return page.data + record_offset + sizeof(RecordHeader);
}

const uint8_t* slot_value(Page& page, uint16_t slot_index, uint16_t& value_len) {
    uint16_t record_offset = *slot_ptr(page, slot_index);
    RecordHeader* record_header = reinterpret_cast<RecordHeader*>(page.data + record_offset);

    value_len = record_header->value_size;
    return page.data + record_offset + sizeof(RecordHeader) + record_header->key_size;
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

    if (index >= header->cell_count || header->cell_count == 0) {
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