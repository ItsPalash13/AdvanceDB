#include "storage/disk_manager.hpp"
#include "common/constants.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

DiskManager::DiskManager(const std::string& file_path) {
    // 0644 is the permission setting for the created file
    // each number represents user, group, others 
    file_descriptor = open(file_path.c_str(), O_RDWR | O_CREAT, 0644);

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

void DiskManager::read_page(int page_id, char* page_data) {
    long offset = static_cast<long>(page_id * PAGE_SIZE);
    if (lseek(file_descriptor, offset, SEEK_SET) < 0) {
        throw std::runtime_error("Failed to seek to the correct position for reading");
    }
    ssize_t bytes_read = read(file_descriptor, page_data, PAGE_SIZE); // read after lseek
    std::cout << "Read " << bytes_read << " bytes from page " << page_id << std::endl;

    if (bytes_read < PAGE_SIZE) {
        std::fill_n(page_data + bytes_read, PAGE_SIZE - bytes_read, 0); // zero the rest
    }
}

void DiskManager::write_page(int page_id, const void* page_data) {
    long offset = static_cast<long>(page_id * PAGE_SIZE); 

    if (lseek(file_descriptor, offset, SEEK_SET) < 0) {
        throw std::runtime_error("Failed to seek to the correct position for writing");
    }
    ssize_t bytes_written = write(file_descriptor, page_data, PAGE_SIZE); 

    std::cout << "Wrote " << bytes_written << " bytes to page " << page_id << std::endl;

    if (bytes_written != PAGE_SIZE) {
        throw std::runtime_error("Failed to write the complete page");
    }
}

void DiskManager::flush() {
    // https://stackoverflow.com/a/68324129
    // TODO: Use fsync on Unixbased systems
    if (_commit(file_descriptor) < 0) {
        throw std::runtime_error("Failed to flush data to disk");
    }
}