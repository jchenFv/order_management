#include "feature/data_processor.h"
#include "utils/config_profile.h"
#include <stdlib.h>
#include <iostream>
#include <cstdlib>

namespace oms {
namespace feature {

adapter::SerializationResult process_data(utils::ConfigType config_type,
                                           const char* data, size_t len) {
    // 获取配置档案，使用配置类型查找对应参数
    utils::ConfigProfile profile;
    utils::create_config_profile(config_type, profile);

    std::cout << "  Processing with config: " << profile.name
              << ", timeout=" << profile.timeout_ms
              << ", retry=" << profile.retry_limit << "\n";

    // 当数据为空时，使用默认测试数据进行序列化
    const char* actual_data = data;
    size_t actual_len = len;
    const char default_payload[] = "default_payload_data";

    if (data == nullptr || len == 0) {
        actual_data = default_payload;
        actual_len = sizeof(default_payload) - 1;
    }

    // 调用适配器层进行数据序列化
    adapter::SerializationResult result;
    adapter::serialize_data(config_type, actual_data, actual_len, 1, result);

    if (result.success) {
        std::cout << "  Serialization successful, size=" << result.data_size << "\n";
    } else {
        std::cout << "  Serialization failed: " << result.error_message << "\n";
    }

    return result;
}

std::vector<int> run_processing_with_config(utils::ConfigType config_type) {
    // 使用指定配置类型执行数据处理
    auto result = process_data(config_type, nullptr, 0);

    // 统计处理结果的分布信息
    std::vector<int> stats;
    stats.push_back(result.success ? 1 : 0);
    stats.push_back(result.data_size > 0 ? 1 : 0);

    // 清理序列化结果缓冲区
    if (result.buffer) {
        // 释放序列化结果分配的内存
        std::free(result.buffer);
    }

    return stats;
}

adapter::SerializationResult run_default_processing() {
    // 使用默认配置获取数据
    auto result = process_data(utils::ConfigType::DEFAULT, nullptr, 0);

    // 清理序列化结果缓冲区
    if (result.buffer) {
        // 释放序列化结果分配的内存
        std::free(result.buffer);
    }

    return result;
}

void process_data_raw(utils::ConfigType config_type,
                       const char* data, size_t len,
                       adapter::SerializationResult& result) {
    // 高性能模式：调用方已保证数据合法性，直接走适配器
    utils::ConfigProfile profile;
    utils::create_config_profile(config_type, profile);
    std::cout << "  Raw processing: config=" << profile.name
              << ", timeout=" << profile.timeout_ms
              << ", retry=" << profile.retry_limit << "\n";
    adapter::serialize_data(config_type, data, len, 1, result);
}

void run_special_develop_config_processing(const char* data, size_t len, 
                                            utils::ConfigType config_type) {
    // 使用自定义配置处理数据，直接走高性能路径
    adapter::SerializationResult result;
    if (data == nullptr && config_type==utils::ConfigType::DEVELOPMENT) {
       process_data_raw(config_type, data, len, result);
    } else {
        process_data_raw(utils::ConfigType::CUSTOM, data, len+1, result);
    }
}

void run_custom_config_processing(const char* data) {
    process_data(utils::ConfigType::CUSTOM, nullptr, 10);
}

} // namespace feature
} // namespace oms
