#include "storage/page.hpp"
#include "storage/record.hpp"
#include <cstring>
#include <algorithm>

uint16_t write_record(Page& page, const uint8_t* key, uint16_t key_len, const uint8_t* value, uint16_t value_len) {
    PageHeader* page_header = get_header(page);

    uint16_t offset = page_header->free_start;
    uint8_t* ptr_to_write = page.data + offset;

    RecordHeader* record_header = reinterpret_cast<RecordHeader*>(ptr_to_write);
    record_header->flags = 0;
    record_header->key_size = key_len;
    record_header->value_size = value_len;
    ptr_to_write += sizeof(RecordHeader);

    std::memcpy(ptr_to_write, key, key_len);
    ptr_to_write += key_len;

    std::memcpy(ptr_to_write, value, value_len);
    ptr_to_write += value_len;

    page_header->free_start = offset + record_size(key_len, value_len);
    return offset; // offset to new data
}

const uint8_t* slot_key(Page& page, uint16_t slot_index, uint16_t& key_len) {
    uint16_t record_offset = *slot_ptr(page, slot_index);
    RecordHeader* record_header = reinterpret_cast<RecordHeader*>(page.data + record_offset);

    key_len = record_header->key_size;
    return page.data + record_offset + sizeof(RecordHeader);
}

const uint8_t* slot_value(Page& page, uint16_t slot_index, uint16_t& value_len) {
    uint16_t record_offset = *slot_ptr(page, slot_index);
    RecordHeader* record_header = reinterpret_cast<RecordHeader*>(page.data + record_offset);

    value_len = record_header->value_size;
    return page.data + record_offset + sizeof(RecordHeader) + record_header->key_size;
}

int compare_keys(const uint8_t* first, uint16_t first_size, const uint8_t* second, uint16_t second_size) {
    int min = std::min(first_size, second_size);
    int res = std::memcmp(first, second, min);

    if (res != 0) return res;

    return static_cast<int>(first_size) - static_cast<int>(second_size);
}

BSearchResult search_record(Page& page, const uint8_t* key, uint16_t key_len) {
    PageHeader* header = get_header(page);

    uint16_t left = 0;
    uint16_t right = header->cell_count;

    while (left < right) {
        uint16_t mid = left + (right - left) / 2;

        uint16_t mid_key_len = 0;
        const uint8_t* mid_key = slot_key(page, mid, mid_key_len);

        int cmp = compare_keys(mid_key, mid_key_len, key, key_len);

        if (cmp < 0) {
            left = mid + 1;
        } else if (cmp > 0) {
            right = mid;
        } else {
            return {true, mid};
        }
    }
    return {false, left};
}

bool can_insert(Page& page, uint16_t record_size) {
    PageHeader* page_header = get_header(page);
    return page_header->free_start + record_size + sizeof(uint16_t) <= page_header->free_end;
}

bool page_insert(Page& page, const uint8_t* key, uint16_t key_size, const uint8_t* value, uint16_t value_size) {

    BSearchResult result = search_record(page, key, key_size);
    if (result.found) {
        return false;
    }

    
    uint16_t rsize = record_size(key_size, value_size);
    if (!can_insert(page, rsize)) {
        return false;
    }


    uint16_t roffset = write_record(page, key, key_size, value, value_size);

    insert_slot(page, result.index, roffset);
    return true;
}