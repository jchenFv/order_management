#include "common/config.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace oms {
namespace config {

static std::unique_ptr<AppConfig> g_global_config;

AppConfig::AppConfig() {
    server_.port = 8080;
    server_.worker_threads = 4;
    server_.enable_metrics = true;
    server_.metrics_endpoint = "/metrics";

    database_.host = "localhost";
    database_.port = 3306;
    database_.max_connections = 20;
    database_.connection_timeout_sec = 30;

    redis_.host = "localhost";
    redis_.port = 6379;
    redis_.max_connections = 50;
    redis_.connect_timeout_ms = 1000;
    redis_.socket_timeout_ms = 5000;

    security_.session_timeout_sec = 3600 * 24;
    security_.password_min_length = 8;
    security_.require_strong_password = true;
    security_.max_login_attempts = 5;
    security_.lockout_duration_sec = 1800;

    payment_.payment_timeout_min = 30;
    payment_.enable_test_mode = true;

    notification_.enable_email = true;
    notification_.enable_sms = true;
    notification_.notification_retry_count = 3;
    notification_.retry_delay_sec = 60;

    inventory_.low_stock_threshold = 10;
    inventory_.reservation_timeout_sec = 900;
    inventory_.auto_release_expired_reservation = true;

    order_.order_timeout_min = 30;
    order_.enable_auto_cancel = true;
    order_.max_items_per_order = 100;
    order_.enable_coupon = true;
    order_.enable_points = true;
    order_.point_exchange_rate = 100;

    audit_.enable_audit_log = true;
    audit_.log_all_queries = false;
    audit_.retention_days = 90;
}

Result AppConfig::load_from_file(const std::string& path) {
    (void)path;
    return Result::ok();
}

Result AppConfig::load_from_env() {
    return Result::ok();
}

Result AppConfig::validate() const {
    if (database_.host.empty()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Database host cannot be empty");
    }
    if (server_.port <= 0 || server_.port > 65535) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Invalid server port");
    }
    return Result::ok();
}

const DatabaseConfig& AppConfig::database() const { return database_; }
const RedisConfig& AppConfig::redis() const { return redis_; }
const ServerConfig& AppConfig::server() const { return server_; }
const SecurityConfig& AppConfig::security() const { return security_; }
const PaymentConfig& AppConfig::payment() const { return payment_; }
const NotificationConfig& AppConfig::notification() const { return notification_; }
const InventoryConfig& AppConfig::inventory() const { return inventory_; }
const OrderConfig& AppConfig::order() const { return order_; }
const AuditConfig& AppConfig::audit() const { return audit_; }

void AppConfig::set_database(const DatabaseConfig& config) { database_ = config; }
void AppConfig::set_redis(const RedisConfig& config) { redis_ = config; }
void AppConfig::set_server(const ServerConfig& config) { server_ = config; }
void AppConfig::set_security(const SecurityConfig& config) { security_ = config; }
void AppConfig::set_payment(const PaymentConfig& config) { payment_ = config; }
void AppConfig::set_notification(const NotificationConfig& config) { notification_ = config; }
void AppConfig::set_inventory(const InventoryConfig& config) { inventory_ = config; }
void AppConfig::set_order(const OrderConfig& config) { order_ = config; }
void AppConfig::set_audit(const AuditConfig& config) { audit_ = config; }

std::string AppConfig::to_string() const {
    std::ostringstream oss;
    oss << "OMS Configuration:\n";
    oss << "  Server: " << server_.bind_address << ":" << server_.port << "\n";
    oss << "  Workers: " << server_.worker_threads << "\n";
    oss << "  Database: " << database_.host << ":" << database_.port << "\n";
    oss << "  Redis: " << redis_.host << ":" << redis_.port << "\n";
    oss << "  Session timeout: " << security_.session_timeout_sec << "s\n";
    oss << "  Payment timeout: " << payment_.payment_timeout_min << "min\n";
    oss << "  Order timeout: " << order_.order_timeout_min << "min\n";
    return oss.str();
}

AppConfig& global_config() {
    if (!g_global_config) {
        g_global_config = std::make_unique<AppConfig>();
    }
    return *g_global_config;
}

Result load_global_config(const std::string& config_file) {
    (void)config_file;
    if (!g_global_config) {
        g_global_config = std::make_unique<AppConfig>();
    }
    return g_global_config->validate();
}

} // namespace config
} // namespace oms
