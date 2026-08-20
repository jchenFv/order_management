#ifndef OMS_ADAPTER_DATA_ADAPTER_H
#define OMS_ADAPTER_DATA_ADAPTER_H

#include "utils/config_profile.h"
#include <string>
#include <cstddef>

namespace oms {
namespace adapter {

/**
 * 数据序列化结果，包含输出缓冲区和状态信息
 */
struct SerializationResult {
    char* buffer;
    size_t data_size;
    std::string error_message;
    bool success;
};

/**
 * 将输入数据按指定格式序列化
 * @param config_type 配置类型，用于获取序列化参数
 * @param data 输入数据
 * @param len 输入数据长度
 * @param format_type 序列化格式类型
 * @param result 输出参数，序列化结果
 */
void serialize_data(utils::ConfigType config_type,
                    const char* data, size_t len,
                    int format_type,
                    SerializationResult& result);

/**
 * 释放序列化结果中的资源
 */
void free_serialization_result(SerializationResult& result);

} // namespace adapter
} // namespace oms

#endif // OMS_ADAPTER_DATA_ADAPTER_H
