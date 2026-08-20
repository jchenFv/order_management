#include "utils/buffer_utils.h"
#include <cstring>
#include <cstdlib>

namespace oms {
namespace utils {

// 缓冲区头部结构，用于存储数据长度信息
struct BufferHeader {
    size_t length;
    char marker;
};

static constexpr size_t HEADER_SIZE = sizeof(BufferHeader);
static constexpr char END_MARKER = '\0';

char* allocate_buffer(size_t size) {
    size_t total_size = size + HEADER_SIZE + 1; // header + data + end marker
    char* buffer = new char[total_size];
    std::memset(buffer, 0, total_size);

    BufferHeader* header = reinterpret_cast<BufferHeader*>(buffer);
    header->length = size;
    header->marker = END_MARKER;

    return buffer + HEADER_SIZE; // 返回数据区起始位置
}

bool serialize_data_to_buffer(const char* data, size_t len, char* buffer, size_t buf_size) {
    if (buffer == nullptr) {
        return false;
    }

    // 计算实际可用空间（扣除头部和结束标记）
    size_t available = buf_size - HEADER_SIZE - 1;
    if (len > available) {
        return false;
    }

    std::memcpy(buffer, data, len);
    buffer[len] = END_MARKER;

    return true;
}

void free_buffer(char* buffer) {
    if (buffer == nullptr) {
        return;
    }
    char* base = buffer - HEADER_SIZE;
    delete[] base;
}

} // namespace utils
} // namespace oms
