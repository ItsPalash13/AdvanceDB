#include "storage/table_handle.hpp"
#include "storage/page.hpp"
#include <sys/stat.h>
#include <stdexcept>
#include <direct.h> // _mkdir
#include <cerrno>

bool open_table(const std::string &name, TableHandle &th) {
    th.table_name = name;
    th.file_path = "data/" + name + ".db";

    struct stat buffer;
    if (stat(th.file_path.c_str(), &buffer) != 0) {
        return false;
    }

    try {
        // TODO FIX 
        th.dm = DiskManager(th.file_path);

        Page meta;
        th.dm.read_page(0, reinterpret_cast<char *>(meta.data));

        PageHeader *ph = get_header(meta);
        th.root_page = ph->root_page;
        return true;
    }
    catch (const std::exception &) {
        return false;
    }
}

bool create_table(const std::string &name) {
    std::string path = "data/" + name + ".db";

    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return false;
    }

    try {
        if (_mkdir("data") != 0 && errno != EEXIST) {
            return false;
        }

        DiskManager dm(path);

        Page meta;
        init_page(meta, 0, PageType::META, PageLevel::NONE);

        Page bitmap;
        init_page(bitmap, 1, PageType::META, PageLevel::NONE);

        uint8_t *bm = bitmap.data + sizeof(PageHeader);

        bm[0] |= (1 << 0);
        bm[0] |= (1 << 1);
        bm[0] |= (1 << 2);
        Page root;
        init_page(root, 2, PageType::DATA, PageLevel::LEAF);

        PageHeader *h = get_header(meta);
        h->root_page = 2;

        dm.write_page(0, meta.data);
        dm.write_page(1, bitmap.data);
        dm.write_page(2, root.data);
        dm.flush();

        return true;
    }
    catch (const std::exception &) {
        return false;
    }
}

uint32_t allocate_page(TableHandle &th) {
    Page bitmap;
    th.dm.read_page(1, reinterpret_cast<char *>(bitmap.data));

    uint8_t *bm = bitmap.data + sizeof(PageHeader);
    uint32_t bitmap_size = PAGE_SIZE - sizeof(PageHeader);

    // Scan bitmap byte by byte, bit by bit
    for (uint32_t byte_idx = 0; byte_idx < bitmap_size; byte_idx++) {
        uint8_t byte = bm[byte_idx];

        // Check each bit in the byte
        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            uint32_t page_id = byte_idx * 8 + bit_idx;

            // Skip pages 0, 1, 2 (meta, bitmap, root)
            if (page_id < 3) {
                continue;
            }

            // Check if bit is free (0 = free, 1 = allocated)
            if ((byte & (1 << bit_idx)) == 0) {
                // Mark as allocated
                bm[byte_idx] |= (1 << bit_idx);
                th.dm.write_page(1, reinterpret_cast<const void *>(bitmap.data));
                th.dm.flush();
                return page_id;
            }
        }
    }

    // No free page found
    return INVALID_PAGE_ID;
}

void free_page(TableHandle &th, uint32_t page_id) {
    Page bitmap;
    th.dm.read_page(1, reinterpret_cast<char *>(bitmap.data));

    uint8_t *bm = bitmap.data + sizeof(PageHeader);

    uint32_t bitmap_size = PAGE_SIZE - sizeof(PageHeader);
    uint32_t byte_idx = page_id / 8;
    uint32_t bit_idx = page_id % 8;
    bm[byte_idx] &= ~(1 << bit_idx);
    th.dm.write_page(1, reinterpret_cast<const void *>(bitmap.data));
    th.dm.flush();
}