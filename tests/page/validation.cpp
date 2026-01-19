#include "storage/page.hpp"
#include "storage/disk_manager.hpp"
#include <iostream>
#include <cstdlib>

void validate_page() {
    Page* page = (Page*)malloc(sizeof(Page));

    init_page(*page, 1, PageType::DATA, PageLevel::INTERNAL);

    std::string storage_path = "G://advancedb//AdvanceDB//database.db";
    DiskManager D = DiskManager(storage_path);

    D.write_page(0, page);

    D.flush();

    char* data = (char *) malloc(sizeof(Page)); 

    D.read_page(0, data);
    PageHeader* ph = (PageHeader*)(data); 

    std::cout << "The page type is " << static_cast<int>(ph->page_type) << " and the page level is " << static_cast<int>(ph->page_level) << std::endl;

    free(page);
    free(data);
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