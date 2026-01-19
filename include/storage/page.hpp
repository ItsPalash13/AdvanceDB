#include <cstdint>
#include "common/constants.hpp"

enum class PageType : uint16_t {
    HEADER = 0,
    META = 1,
    INDEX = 2,
    DATA = 3,
    FREE = 4
};

enum class PageLevel : uint16_t {
    LEAF = 0,
    INTERNAL = 1
};

// ensuring single byte of alignment for each field in the struct
#pragma pack(push, 1)
struct PageHeader {
    uint32_t page_id;
    PageType page_type;
    PageLevel page_level;

    // might need to take a look at struct again
    uint8_t reserved[8];
    uint16_t flags;
    uint16_t cell_count; 
    uint16_t free_start;
    uint16_t free_end;

    uint32_t parent_page_id;
    uint32_t lsn; 
};
#pragma pack(pop)


struct Page {
    uint8_t data[PAGE_SIZE];
};


static_assert(sizeof(PageHeader) ==  32, "PageHeader size must be 32 bytes");
// get header of a page 
inline PageHeader* get_header(Page& page);

// Page functionalities
void init_page(Page& page, uint32_t page_id, PageType page_type, PageLevel page_level);

// slot functionalities
uint16_t* slot_ptr(Page& page, uint16_t index);
void insert_slot(Page& page, uint16_t index, uint16_t record_offset);
void remove_slot(Page& page, uint16_t index);
