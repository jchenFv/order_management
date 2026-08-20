#ifndef OMS_UTILS_CONFIG_PROFILE_H
#define OMS_UTILS_CONFIG_PROFILE_H

#include <string>
#include <map>

namespace oms {
namespace utils {

enum class ConfigType {
    DEFAULT = 0,
    PRODUCTION = 1,
    DEVELOPMENT = 2,
    CUSTOM = 3,
};

struct ConfigProfile {
    ConfigType type;
    std::string name;
    std::string host;
    int port;
    int timeout_ms;
    int retry_limit;
    bool enabled;
};

/**
 * 根据配置类型填充配置档案，从预设配置表中匹配
 * 如果找不到匹配项，仅部分初始化该配置
 */
void create_config_profile(ConfigType config_type, ConfigProfile& profile);

/**
 * 获取当前环境中可用的配置类型列表
 */
std::map<ConfigType, ConfigProfile> list_available_profiles();

/**
 * 将配置类型转换为可读字符串
 */
const char* config_type_to_string(ConfigType type);

} // namespace utils
} // namespace oms

#endif // OMS_UTILS_CONFIG_PROFILE_H
