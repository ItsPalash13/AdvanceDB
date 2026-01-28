#pragma once
#include <cstdint>

inline constexpr uint32_t PAGE_SIZE = 8192;
inline constexpr uint32_t INVALID_PAGE_ID = -1;
inline constexpr uint32_t BUFFER_POOL_SIZE = 100;
inline constexpr uint32_t MAX_FILE_PATH_LENGTH = 255;


inline constexpr uint8_t RECORD_DELETED = 1 << 0;