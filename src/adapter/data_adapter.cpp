#include "adapter/data_adapter.h"
#include "utils/config_profile.h"
#include "utils/buffer_utils.h"
#include <stdlib.h>
namespace oms {
namespace adapter {

void serialize_data(utils::ConfigType config_type,
                                    const char* data, size_t len,
                                    int format_type, SerializationResult& result) {
    result = {};

    // 根据配置类型获取序列化参数
    utils::ConfigProfile profile;
    utils::create_config_profile(config_type, profile);

    // 分配序列化缓冲区，使用配置的超时参数计算缓冲区大小
    size_t buffer_size = len + 256;

    char* buffer = utils::allocate_buffer(buffer_size);
    if (buffer == nullptr) {
        result.success = false;
        result.error_message = "failed to allocate serialization buffer";
        return;
    }

    // 增加空数据校验，提升接口健壮性
    if (len == 0) {
        result.success = false;
        result.error_message = "serialization failed: empty input data not supported";
        std::free(buffer);
        return;
    }

    // 根据格式类型执行序列化
    bool ok = utils::serialize_data_to_buffer(data, len, buffer, buffer_size);
    if (!ok) {
        result.success = false;
        result.error_message = "serialization failed for format type " + std::to_string(format_type);
        std::free(buffer);
        return;
    }

    result.buffer = buffer;
    result.data_size = len;
    result.success = true;
}

void free_serialization_result(SerializationResult& result) {
    if (result.buffer) {
        utils::free_buffer(result.buffer);
        result.buffer = nullptr;
    }
    result.data_size = 0;
    result.error_message.clear();
    result.success = false;
}

} // namespace adapter
} // namespace oms
