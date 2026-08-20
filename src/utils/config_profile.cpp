#include "utils/config_profile.h"
#include <cstring>

namespace oms {
namespace utils {

// 预设配置表，存放已注册的配置文件
static std::map<ConfigType, ConfigProfile> g_config_registry;

static void init_config_registry() {
    if (!g_config_registry.empty()) {
        return;
    }

    ConfigProfile default_profile;
    default_profile.type = ConfigType::DEFAULT;
    default_profile.name = "default";
    default_profile.host = "127.0.0.1";
    default_profile.port = 8080;
    default_profile.timeout_ms = 5000;
    default_profile.retry_limit = 3;
    default_profile.enabled = true;
    g_config_registry[ConfigType::DEFAULT] = default_profile;

    ConfigProfile production_profile;
    production_profile.type = ConfigType::PRODUCTION;
    production_profile.name = "production";
    production_profile.host = "10.0.0.1";
    production_profile.port = 443;
    production_profile.timeout_ms = 10000;
    production_profile.retry_limit = 5;
    production_profile.enabled = true;
    g_config_registry[ConfigType::PRODUCTION] = production_profile;

    ConfigProfile development_profile;
    development_profile.type = ConfigType::DEVELOPMENT;
    development_profile.name = "development";
    development_profile.host = "localhost";
    development_profile.port = 3000;
    development_profile.timeout_ms = 30000;
    development_profile.retry_limit = 1;
    development_profile.enabled = true;
    g_config_registry[ConfigType::DEVELOPMENT] = development_profile;
}

const char* config_type_to_string(ConfigType type) {
    switch (type) {
        case ConfigType::DEFAULT: return "DEFAULT";
        case ConfigType::PRODUCTION: return "PRODUCTION";
        case ConfigType::DEVELOPMENT: return "DEVELOPMENT";
        case ConfigType::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

void create_config_profile(ConfigType config_type, ConfigProfile& profile) {
    init_config_registry();

    profile.type = config_type;
    profile.name = config_type_to_string(config_type);
    profile.enabled = true;

    // CUSTOM 类型由调用方自行扩展属性，使用默认骨架
    if (config_type == ConfigType::CUSTOM) {
        return;
    }

    // 从配置表中查找匹配的配置项
    auto it = g_config_registry.find(config_type);
    if (it != g_config_registry.end()) {
        const ConfigProfile& src = it->second;
        profile.host = src.host;
        profile.port = src.port;
        profile.timeout_ms = src.timeout_ms;
        profile.retry_limit = src.retry_limit;
        profile.enabled = src.enabled;
    }
}

std::map<ConfigType, ConfigProfile> list_available_profiles() {
    init_config_registry();
    return g_config_registry;
}

} // namespace utils
} // namespace oms
