#include <string>
#include "storage/disk_manager.hpp"
#include <cstdint>


struct TableHandle {
    std::string table_name;
    std::string file_path;

    DiskManager dm;

    uint32_t root_page;

    TableHandle() = default;

    explicit TableHandle(const std::string& name)
        : table_name(name),
          file_path("data/" + name + ".db"),
          dm(file_path),
          root_page(0)
    {}
};

bool open_table(const std::string &name, TableHandle &th);
bool create_table(const std::string &name);
uint32_t allocate_page(TableHandle &th);
void free_page(TableHandle &th, uint32_t page_id);
