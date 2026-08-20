#ifndef OMS_COMMON_RESULT_H
#define OMS_COMMON_RESULT_H

#include <string>
#include <system_error>
#include <variant>

namespace oms {

enum class ErrorCode {
    SUCCESS = 0,

    // 通用错误 1000-1999
    INVALID_PARAMETER = 1001,
    NULL_POINTER = 1002,
    OUT_OF_RANGE = 1003,
    NOT_IMPLEMENTED = 1004,
    INTERNAL_ERROR = 1005,
    TIMEOUT = 1006,
    NETWORK_ERROR = 1007,

    // 认证授权 2000-2999
    AUTH_REQUIRED = 2001,
    AUTH_FAILED = 2002,
    AUTH_EXPIRED = 2003,
    PERMISSION_DENIED = 2004,
    ROLE_REQUIRED = 2005,
    INVALID_SESSION = 2006,
    SESSION_EXPIRED = 2007,

    // 用户相关 3000-3999
    USER_NOT_FOUND = 3001,
    USER_ALREADY_EXISTS = 3002,
    USER_DISABLED = 3003,
    INVALID_PASSWORD = 3004,
    PASSWORD_TOO_WEAK = 3005,

    // 订单相关 4000-4999
    ORDER_NOT_FOUND = 4001,
    ORDER_ALREADY_PAID = 4002,
    ORDER_STATUS_INVALID = 4003,
    ORDER_ITEM_EMPTY = 4004,
    ORDER_CANNOT_CANCEL = 4005,
    ORDER_AMOUNT_MISMATCH = 4006,

    // 库存相关 5000-5999
    PRODUCT_NOT_FOUND = 5001,
    PRODUCT_OUT_OF_STOCK = 5002,
    PRODUCT_LOW_STOCK = 5003,
    INVENTORY_RESERVE_FAILED = 5004,
    INVENTORY_RELEASE_FAILED = 5005,
    WAREHOUSE_NOT_FOUND = 5006,

    // 支付相关 6000-6999
    PAYMENT_FAILED = 6001,
    PAYMENT_TIMEOUT = 6002,
    PAYMENT_AMOUNT_MISMATCH = 6003,
    PAYMENT_ALREADY_REFUNDED = 6004,
    PAYMENT_GATEWAY_ERROR = 6005,
    PAYMENT_INVALID_STATUS = 6006,
    PAYMENT_REFUND_EXCEEDS_AMOUNT = 6007,
    PAYMENT_INVALID_AMOUNT = 6008,
    PAYMENT_METHOD_NOT_SUPPORTED = 6009,
    PAYMENT_GATEWAY_NOT_FOUND = 6010,
    PAYMENT_INVALID_CARD = 6011,
    PAYMENT_NOT_FOUND = 6012,
    PAYMENT_REFUND_NOT_FOUND = 6013,

    // 折扣促销 7000-7999
    DISCOUNT_NOT_FOUND = 7001,
    DISCOUNT_EXPIRED = 7002,
    DISCOUNT_NOT_APPLICABLE = 7003,
    DISCOUNT_USAGE_LIMIT_REACHED = 7004,

    // 数据库相关 8000-8999
    DB_CONNECTION_FAILED = 8001,
    DB_QUERY_FAILED = 8002,
    DB_TRANSACTION_FAILED = 8003,
    DB_TRANSACTION_ERROR = 8004,
    DB_DEADLOCK = 8005,
    DB_CONSTRAINT_VIOLATION = 8006,

    // 缓存相关 9000-9999
    CACHE_MISS = 9001,
    CACHE_TIMEOUT = 9002,
    CACHE_SERIALIZATION_FAILED = 9003,
    CACHE_ERROR = 9004,

    // 通知相关 10000-10999
    NOTIFICATION_FAILED = 10001,
    NOTIFICATION_CHANNEL_DISABLED = 10002,
    NOTIFICATION_RATE_LIMITED = 10003
};

class Result {
public:
    Result() : code_(ErrorCode::SUCCESS) {}
    Result(ErrorCode code) : code_(code) {}
    Result(ErrorCode code, const std::string& msg) : code_(code), message_(msg) {}

    static Result ok() { return Result(ErrorCode::SUCCESS); }
    static Result error(ErrorCode code, const std::string& msg = "") {
        return Result(code, msg);
    }

    operator bool() const { return code_ == ErrorCode::SUCCESS; }
    bool is_success() const { return code_ == ErrorCode::SUCCESS; }
    bool is_error() const { return code_ != ErrorCode::SUCCESS; }

    ErrorCode code() const { return code_; }
    ErrorCode error_code() const { return code_; }
    const std::string& message() const { return message_; }
    const std::string& error_message() const { return message_; }

    bool operator==(const Result& other) const {
        return code_ == other.code_;
    }

    bool operator!=(const Result& other) const {
        return !(*this == other);
    }

private:
    ErrorCode code_;
    std::string message_;
};

template<typename T>
class ResultT {
public:
    ResultT() : success_(false) {}
    ResultT(const T& value) : success_(true), value_(value) {}
    ResultT(ErrorCode code, const std::string& msg = "")
        : success_(false), error_code_(code), error_message_(msg) {}

    static ResultT<T> ok(const T& value) { return ResultT<T>(value); }
    static ResultT<T> error(ErrorCode code, const std::string& msg = "") {
        return ResultT<T>(code, msg);
    }

    operator bool() const { return success_; }
    bool is_success() const { return success_; }

    const T& value() const { return *value_; }
    T& value() { return *value_; }

    ErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }

private:
    bool success_;
    std::optional<T> value_;
    ErrorCode error_code_;
    std::string error_message_;
};

template<>
class ResultT<void> {
public:
    ResultT() : success_(true) {}
    ResultT(ErrorCode code, const std::string& msg = "")
        : success_(false), error_code_(code), error_message_(msg) {}

    static ResultT<void> ok() { return ResultT<void>(); }
    static ResultT<void> error(ErrorCode code, const std::string& msg = "") {
        return ResultT<void>(code, msg);
    }

    operator bool() const { return success_; }
    bool is_success() const { return success_; }

    ErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }

private:
    bool success_;
    ErrorCode error_code_;
    std::string error_message_;
};

inline std::string error_code_to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::INVALID_PARAMETER: return "INVALID_PARAMETER";
        case ErrorCode::NULL_POINTER: return "NULL_POINTER";
        case ErrorCode::AUTH_REQUIRED: return "AUTH_REQUIRED";
        case ErrorCode::PERMISSION_DENIED: return "PERMISSION_DENIED";
        case ErrorCode::USER_NOT_FOUND: return "USER_NOT_FOUND";
        case ErrorCode::ORDER_NOT_FOUND: return "ORDER_NOT_FOUND";
        case ErrorCode::PRODUCT_NOT_FOUND: return "PRODUCT_NOT_FOUND";
        case ErrorCode::PRODUCT_OUT_OF_STOCK: return "PRODUCT_OUT_OF_STOCK";
        case ErrorCode::DB_CONNECTION_FAILED: return "DB_CONNECTION_FAILED";
        default: return "UNKNOWN_ERROR";
    }
}

} // namespace oms

#endif // OMS_COMMON_RESULT_H
