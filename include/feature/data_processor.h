#ifndef OMS_FEATURE_DATA_PROCESSOR_H
#define OMS_FEATURE_DATA_PROCESSOR_H

#include "adapter/data_adapter.h"
#include "utils/config_profile.h"
#include <string>
#include <vector>

namespace oms {
namespace feature {

/**
 * 处理输入数据并返回序列化结果
 * @param config_type 配置类型
 * @param data 输入数据（可为空表示使用默认数据）
 * @param len 数据长度
 * @return 序列化结果
 */
adapter::SerializationResult process_data(utils::ConfigType config_type,
                                           const char* data, size_t len);

/**
 * 使用指定配置类型批量处理数据，返回各状态统计
 * @param config_type 配置类型
 * @return 统计结果向量
 */
std::vector<int> run_processing_with_config(utils::ConfigType config_type);

/**
 * 使用默认配置处理数据
 */
adapter::SerializationResult run_default_processing();

} // namespace feature
} // namespace oms

#endif // OMS_FEATURE_DATA_PROCESSOR_H
