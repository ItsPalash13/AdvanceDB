#include "storage/page.hpp"
#include "storage/disk_manager.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cassert>

void debug_print_slot(Page& page) {
    auto* header = reinterpret_cast<PageHeader*>(page.data);

    if (header->cell_count == 0) {
        std::cout << "No slots to print" << std::endl;
        return;
    }

    std::cout << "Page slots (cell_count: " << header->cell_count << "):" << std::endl;
    for (uint16_t i = 0; i < header->cell_count; ++i) {
        uint16_t* slot = slot_ptr(page, i);
        std::cout << "  Slot[" << i << "] = " << *slot << std::endl;
    }
}
void validate_page() {
    Page page;
    init_page(page, 0, PageType::DATA, PageLevel::LEAF);

    insert_slot(page, 0, 12);
    insert_slot(page, 1, 13);
    insert_slot(page, 2, 14);

    std::cout << "Slots BEFORE disk write:\n";
    debug_print_slot(page);

    remove_slot(page, 1);
    std::cout << "Slots AFTER removal:\n";
    debug_print_slot(page);

    std::string storage_path = "G://advancedb//AdvanceDB//database.db";
    DiskManager dm(storage_path);

    dm.write_page(0, page.data);
    dm.flush();

    Page page2;
    dm.read_page(0, reinterpret_cast<char*>(page2.data));

    std::cout << "Slots AFTER disk read:\n";
    debug_print_slot(page2);

    // 5. Validate invariants
    PageHeader* h = get_header(page2);
    assert(h->cell_count == 2); 
    assert(h->free_start <= h->free_end);
}


int main() {
    try {
        validate_page();
        std::cout << "Validation completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}