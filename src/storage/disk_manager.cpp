#include "storage/disk_manager.hpp"
#include "common/constants.hpp"
#include "storage/page.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <algorithm>
#ifdef _WIN32
#include <io.h>
#endif

DiskManager::DiskManager(const std::string& file_path) {
    // 0644 is the permission setting for the created file
    // each number represents user, group, others 
    // Use O_BINARY on Windows to avoid text mode translation
    #ifdef _WIN32
    file_descriptor = open(file_path.c_str(), O_RDWR | O_CREAT | O_BINARY, 0644);
    #else
    file_descriptor = open(file_path.c_str(), O_RDWR | O_CREAT, 0644);
    #endif

    if (file_descriptor < 0) {
        throw std::runtime_error("Failed to open or create file");
    }
}

DiskManager::DiskManager(DiskManager&& other) noexcept {
    file_descriptor = other.file_descriptor;
    other.file_descriptor = -1;
}

DiskManager& DiskManager::operator=(DiskManager&& other) noexcept {
    if (this == &other) return *this;
    if (file_descriptor >= 0) {
        close(file_descriptor);
    }
    file_descriptor = other.file_descriptor;
    other.file_descriptor = -1;
    return *this;
}

DiskManager::~DiskManager() {
    if (file_descriptor >= 0) {
        close(file_descriptor);
    }
}

void DiskManager::read_page(int page_id, uint8_t* page_data) {
    long offset = static_cast<long>(page_id * PAGE_SIZE);
    if (lseek(file_descriptor, offset, SEEK_SET) < 0) {
        throw std::runtime_error("Failed to seek to the correct position for reading");
    }
    
    // Read in a loop to ensure we get all bytes (in case of partial reads)
    ssize_t total_read = 0;
    ssize_t bytes_read = 0;
    uint8_t* ptr = page_data;
    
    while (total_read < PAGE_SIZE) {
        bytes_read = read(file_descriptor, ptr + total_read, PAGE_SIZE - total_read);
        if (bytes_read < 0) {
            throw std::runtime_error("Failed to read page data");
        }
        if (bytes_read == 0) {
            // EOF reached - zero the rest
            break;
        }
        total_read += bytes_read;
    }
    
    std::cout << "Read " << total_read << " bytes from page " << page_id << std::endl;

    if (total_read < PAGE_SIZE) {
        // Zero the rest if we didn't read the full page
        std::fill_n(page_data + total_read, PAGE_SIZE - total_read, 0);
    }
}

void DiskManager::write_page(int page_id, const void* page_data) {
    long offset = static_cast<long>(page_id * PAGE_SIZE);
    long required_size = offset + PAGE_SIZE;
    
    // Ensure file is large enough - seek to end and extend if needed
    long current_size = lseek(file_descriptor, 0, SEEK_END);
    if (current_size < 0) {
        throw std::runtime_error("Failed to get file size");
    }
    
    if (current_size < required_size) {
        // Extend file by writing a zero byte at the required position
        if (lseek(file_descriptor, required_size - 1, SEEK_SET) < 0) {
            throw std::runtime_error("Failed to seek to extend file");
        }
        char zero = 0;
        ssize_t extend_bytes = write(file_descriptor, &zero, 1);
        if (extend_bytes != 1) {
            throw std::runtime_error("Failed to extend file");
        }
        // Flush after extending
        _commit(file_descriptor);
    }

    if (lseek(file_descriptor, offset, SEEK_SET) < 0) {
        throw std::runtime_error("Failed to seek to the correct position for writing");
    }
    
    ssize_t bytes_written = write(file_descriptor, page_data, PAGE_SIZE); 

    std::cout << "Wrote " << bytes_written << " bytes to page " << page_id << std::endl;

    if (bytes_written != PAGE_SIZE) {
        throw std::runtime_error("Failed to write the complete page");
    }
    
    // Flush to ensure data is written to disk before next read
    // On Windows, we need to use _commit instead of fsync
    _commit(file_descriptor);
}

void DiskManager::flush() {
    // https://stackoverflow.com/a/68324129
    // TODO: Use fsync on Unixbased systems
    if (_commit(file_descriptor) < 0) {
        throw std::runtime_error("Failed to flush data to disk");
    }
}