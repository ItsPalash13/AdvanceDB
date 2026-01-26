#include "storage/page.hpp"
#include "storage/record.hpp"
#include "storage/disk_manager.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <string>

void debug_print_slot(Page &page)
{
    auto *header = reinterpret_cast<PageHeader *>(page.data);

    if (header->cell_count == 0)
    {
        std::cout << "No slots to print" << std::endl;
        return;
    }

    std::cout << "Page slots (cell_count: " << header->cell_count << "):" << std::endl;
    for (uint16_t i = 0; i < header->cell_count; ++i)
    {
        uint16_t *slot = slot_ptr(page, i);
        std::cout << "  Slot[" << i << "] = " << *slot << std::endl;
    }
}

Page* validate_page_insert()
{
    Page* page = new Page();
    init_page(*page, 0, PageType::DATA, PageLevel::LEAF);

    std::cout << "\n--- page_insert test ---\n";
    std::cout << "Inserting (b -> val2)\n";
    bool ok = page_insert(*page,
                          (uint8_t *)"b", 1,
                          (uint8_t *)"val2", 4);
    std::cout << "  result: " << (ok ? "inserted" : "rejected") << "\n";
    assert(ok);

    std::cout << "Inserting (a -> val1)\n";
    ok = page_insert(*page,
                     (uint8_t *)"a", 1,
                     (uint8_t *)"val1", 4);
    std::cout << "  result: " << (ok ? "inserted" : "rejected") << "\n";
    assert(ok);

    std::cout << "Inserting (c -> val3)\n";
    ok = page_insert(*page,
                     (uint8_t *)"c", 1,
                     (uint8_t *)"val3", 4);
    std::cout << "  result: " << (ok ? "inserted" : "rejected") << "\n";
    assert(ok);

    std::cout << "Inserting duplicate (b -> valX) (should be rejected)\n";
    ok = page_insert(*page,
                     (uint8_t *)"b", 1,
                     (uint8_t *)"valX", 4);
    std::cout << "  result: " << (ok ? "inserted (UNEXPECTED)" : "rejected (expected)") << "\n";
    assert(!ok);

    PageHeader *header = get_header(*page);
    std::cout << "cell_count: " << header->cell_count << "\n";
    assert(header->cell_count == 3);

    const char *expected_keys[] = {"a", "b", "c"};
    const char *expected_vals[] = {"val1", "val2", "val3"};

    std::cout << "\nVerifying slot order + payloads:\n";
    for (uint16_t i = 0; i < header->cell_count; ++i)
    {
        uint16_t key_len = 0;
        const uint8_t *key_ptr = slot_key(*page, i, key_len);
        assert(key_len == 1);
        assert(std::memcmp(key_ptr, expected_keys[i], 1) == 0);

        uint16_t val_len = 0;
        const uint8_t *val_ptr = slot_value(*page, i, val_len);
        assert(val_len == 4);
        assert(std::memcmp(val_ptr, expected_vals[i], 4) == 0);

        std::cout << "  slot[" << i << "]: key=" << static_cast<char>(key_ptr[0])
                  << " val=" << std::string(reinterpret_cast<const char *>(val_ptr), val_len) << "\n";
    }

    BSearchResult r = search_record(*page, (const uint8_t *)"b", 1);
    std::cout << "\nsearch_record('b') -> found=" << (r.found ? "true" : "false") << " index=" << r.index << "\n";
    assert(r.found && r.index == 1);

    BSearchResult r2 = search_record(*page, (const uint8_t *)"d", 1);
    std::cout << "search_record('d') -> found=" << (r2.found ? "true" : "false") << " index=" << r2.index << "\n";
    assert(!r2.found && r2.index == 3);

    std::cout << "\nRaw slot offsets:\n";
    debug_print_slot(*page);

    std::string storage_path = "G://advancedb//AdvanceDB//database.db";
    DiskManager dm(storage_path);

    std::cout << "Writing page to disk (page_id = 0)\n";
    dm.write_page(0, page->data);
    dm.flush();

    Page page_from_disk1;
    dm.read_page(0, reinterpret_cast<char*>(page_from_disk1.data));
    std::cout << "After first read from disk:\n";
    debug_print_slot(page_from_disk1);

    Page page_from_disk2;
    dm.read_page(0, reinterpret_cast<char*>(page_from_disk2.data));
    std::cout << "After second read from disk:\n";
    debug_print_slot(page_from_disk2);

    return page;
}

int main()
{
    try
    {
        Page* page = validate_page_insert();
        std::cout << "page_insert validation completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}