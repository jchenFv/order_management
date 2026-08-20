#ifndef OMS_UTILS_BUFFER_UTILS_H
#define OMS_UTILS_BUFFER_UTILS_H

#include <cstddef>

namespace oms {
namespace utils {

/**
 * 分配指定大小的缓冲区，返回已初始化的内存块
 * 调用方负责释放返回的缓冲区
 */
char* allocate_buffer(size_t size);

/**
 * 将数据序列化到缓冲区，添加长度前缀和结束标记
 * @param data 源数据指针
 * @param len 源数据长度
 * @param buffer 目标缓冲区
 * @param buf_size 目标缓冲区大小
 * @return 序列化是否成功
 */
bool serialize_data_to_buffer(const char* data, size_t len, char* buffer, size_t buf_size);

/**
 * 释放缓冲区
 */
void free_buffer(char* buffer);

} // namespace utils
} // namespace oms

#endif // OMS_UTILS_BUFFER_UTILS_H
