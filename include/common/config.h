#ifndef OMS_COMMON_CONFIG_H
#define OMS_COMMON_CONFIG_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <map>

namespace oms {
namespace config {

struct DatabaseConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string database;
    int max_connections;
    int connection_timeout_sec;
    bool enable_ssl;
};

struct RedisConfig {
    std::string host;
    int port;
    std::string password;
    int db_index;
    int max_connections;
    int connect_timeout_ms;
    int socket_timeout_ms;
    bool enable_cluster;
    std::vector<std::string> cluster_nodes;
};

struct ServerConfig {
    std::string bind_address;
    int port;
    int worker_threads;
    int max_request_size;
    bool enable_metrics;
    std::string metrics_endpoint;
};

struct SecurityConfig {
    int session_timeout_sec;
    int password_min_length;
    bool require_strong_password;
    int max_login_attempts;
    int lockout_duration_sec;
    std::string jwt_secret;
    int jwt_expire_hours;
    bool enable_2fa;
};

struct PaymentConfig {
    std::string alipay_app_id;
    std::string alipay_private_key;
    std::string alipay_public_key;
    std::string wechat_app_id;
    std::string wechat_mch_id;
    std::string wechat_api_key;
    int payment_timeout_min;
    bool enable_test_mode;
};

struct NotificationConfig {
    struct SMTPConfig {
        std::string host;
        int port;
        std::string username;
        std::string password;
        bool use_tls;
        std::string from_address;
        std::string from_name;
    };

    struct SMSConfig {
        std::string provider;
        std::string api_key;
        std::string api_secret;
        std::string template_id;
        std::string sign_name;
        int rate_limit_per_minute;
    };

    SMTPConfig smtp;
    SMSConfig sms;
    bool enable_email;
    bool enable_sms;
    bool enable_in_app;
    int notification_retry_count;
    int retry_delay_sec;
};

struct InventoryConfig {
    int low_stock_threshold;
    int default_warehouse_id;
    bool enable_negative_inventory;
    int reservation_timeout_sec;
    bool auto_release_expired_reservation;
};

struct OrderConfig {
    std::string order_id_prefix;
    bool auto_generate_order_id;
    int order_timeout_min;
    bool enable_auto_cancel;
    double max_order_amount;
    int max_items_per_order;
    bool enable_coupon;
    bool enable_points;
    int point_exchange_rate;
};

struct AuditConfig {
    bool enable_audit_log;
    bool log_all_queries;
    bool log_sensitive_data;
    int retention_days;
    std::string log_storage;
    bool enable_alerts;
    std::vector<std::string> alert_channels;
};

class AppConfig {
public:
    AppConfig();

    Result load_from_file(const std::string& path);
    Result load_from_env();
    Result validate() const;

    const DatabaseConfig& database() const;
    const RedisConfig& redis() const;
    const ServerConfig& server() const;
    const SecurityConfig& security() const;
    const PaymentConfig& payment() const;
    const NotificationConfig& notification() const;
    const InventoryConfig& inventory() const;
    const OrderConfig& order() const;
    const AuditConfig& audit() const;

    void set_database(const DatabaseConfig& config);
    void set_redis(const RedisConfig& config);
    void set_server(const ServerConfig& config);
    void set_security(const SecurityConfig& config);
    void set_payment(const PaymentConfig& config);
    void set_notification(const NotificationConfig& config);
    void set_inventory(const InventoryConfig& config);
    void set_order(const OrderConfig& config);
    void set_audit(const AuditConfig& config);

    std::string to_string() const;

private:
    DatabaseConfig database_;
    RedisConfig redis_;
    ServerConfig server_;
    SecurityConfig security_;
    PaymentConfig payment_;
    NotificationConfig notification_;
    InventoryConfig inventory_;
    OrderConfig order_;
    AuditConfig audit_;
};

AppConfig& global_config();
Result load_global_config(const std::string& config_file = "");

} // namespace config
} // namespace oms

#endif // OMS_COMMON_CONFIG_H
