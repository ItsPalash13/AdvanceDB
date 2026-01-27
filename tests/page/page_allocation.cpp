#include <iostream>
#include <cassert>
#include "storage/page.hpp"
#include "storage/record.hpp"
#include "storage/table_handle.hpp"
#include <assert.h>

void test_table_and_page_allocator() {
    std::cout << "=== Storage Engine Core Test ===\n";

    const std::string table = "test_users";

    // Cleanup if exists
    std::string path = "data/" + table + ".db";
    remove(path.c_str());

    // 1. Create table
    assert(create_table(table) && "create_table failed");

    std::cout << "[OK] create_table\n";

    // 2. Open table
    TableHandle th(table);
    assert(open_table(table, th) && "open_table failed");

    std::cout << "[OK] open_table\n";
    std::cout << "Root page: " << th.root_page << "\n";

    // 3. Allocate some pages
    uint32_t p1 = allocate_page(th);
    uint32_t p2 = allocate_page(th);
    uint32_t p3 = allocate_page(th);

    std::cout << "Allocated pages: "
              << p1 << ", " << p2 << ", " << p3 << "\n";

    assert(p1 != INVALID_PAGE_ID);
    assert(p2 != INVALID_PAGE_ID);
    assert(p3 != INVALID_PAGE_ID);
    assert(p1 != p2 && p2 != p3 && p1 != p3);

    std::cout << "[OK] allocate_page\n";

    // 4. Free one page
    free_page(th, p2);

    std::cout << "[OK] free_page(" << p2 << ")\n";

    // 5. Reallocate — should reuse freed page
    uint32_t p4 = allocate_page(th);
    std::cout << "Reallocated page: " << p4 << "\n";

    assert(p4 == p2 && "Free page was not reused");

    std::cout << "[OK] free list reuse verified\n";

    std::cout << "\n=== ALL STORAGE TESTS PASSED ===\n";
}

int main()
{
    try
    {
        test_table_and_page_allocator();
        std::cout << "page_insert validation completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}