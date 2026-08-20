#ifndef OMS_COMMON_TYPES_H
#define OMS_COMMON_TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <variant>
#include <iostream>

namespace oms {

using UserId = uint64_t;
using OrderId = uint64_t;
using ProductId = uint64_t;
using PaymentId = uint64_t;
using TransactionId = uint64_t;
using RefundId = uint64_t;
using WarehouseId = uint64_t;
using SessionId = std::string;
using RoleId = uint32_t;
using PermissionId = uint32_t;
using DiscountId = uint64_t;
using PromotionId = uint64_t;

constexpr UserId INVALID_USER_ID = 0;
constexpr OrderId INVALID_ORDER_ID = 0;
constexpr ProductId INVALID_PRODUCT_ID = 0;
constexpr PaymentId INVALID_PAYMENT_ID = 0;
constexpr WarehouseId INVALID_WAREHOUSE_ID = 0;

enum class OrderStatus {
    CREATED = 1,
    PENDING_PAYMENT = 2,
    PAID = 3,
    PROCESSING = 4,
    SHIPPED = 5,
    DELIVERED = 6,
    COMPLETED = 7,
    CANCELLED = 8,
    REFUNDED = 9,
    CLOSED = 10
};

enum class PaymentMethod {
    UNKNOWN = 0,
    CREDIT_CARD = 1,
    DEBIT_CARD = 2,
    ALIPAY = 3,
    WECHAT_PAY = 4,
    BANK_TRANSFER = 5,
    PAYPAL = 6,
    APPLE_PAY = 7,
    QR_CODE = 8,
    MINI_PROGRAM = 9
};

enum class PaymentStatus {
    PENDING = 1,
    PROCESSING = 2,
    SUCCESS = 3,
    FAILED = 4,
    REFUNDED = 5,
    PARTIAL_REFUND = 6
};

enum class UserRole {
    GUEST = 0,
    CUSTOMER = 1,
    VIP_CUSTOMER = 2,
    STAFF = 3,
    MANAGER = 4,
    FINANCE = 5,
    WAREHOUSE_ADMIN = 6,
    ADMIN = 7,
    SUPER_ADMIN = 8
};

enum class InventoryStatus {
    IN_STOCK = 1,
    LOW_STOCK = 2,
    OUT_OF_STOCK = 3,
    RESERVED = 4,
    DAMAGED = 5,
    IN_TRANSIT = 6
};

enum class DiscountType {
    PERCENTAGE = 1,
    FIXED_AMOUNT = 2,
    BUY_N_GET_M_FREE = 3,
    FULL_REDUCTION = 4
};

enum class NotificationChannel {
    EMAIL = 1,
    SMS = 2,
    IN_APP = 4,
    WECHAT = 8,
    ALL = 15
};

enum class AuditAction {
    USER_LOGIN = 1,
    USER_LOGOUT = 2,
    USER_CREATE = 3,
    USER_UPDATE = 4,
    USER_DELETE = 5,

    ORDER_CREATE = 101,
    ORDER_UPDATE = 102,
    ORDER_CANCEL = 103,
    ORDER_STATUS_CHANGE = 104,
    ORDER_PAYMENT = 105,
    ORDER_REFUND = 106,

    PRODUCT_ADD = 201,
    PRODUCT_UPDATE = 202,
    PRODUCT_DELETE = 203,
    INVENTORY_SYNC = 204,
    INVENTORY_RESERVE = 205,
    INVENTORY_RELEASE = 206,

    DISCOUNT_CREATE = 301,
    DISCOUNT_UPDATE = 302,
    DISCOUNT_DELETE = 303,

    PERMISSION_GRANT = 401,
    PERMISSION_REVOKE = 402,
    ROLE_CREATE = 403,
    ROLE_DELETE = 404,

    DB_QUERY = 501,
    DB_TRANSACTION_BEGIN = 502,
    DB_TRANSACTION_COMMIT = 503,
    DB_TRANSACTION_ROLLBACK = 504,

    CACHE_HIT = 601,
    CACHE_MISS = 602,
    CACHE_INVALIDATE = 603,

    NOTIFICATION_SEND = 701,
    NOTIFICATION_FAILED = 702
};

struct Money {
    int64_t amount;
    std::string currency;

    Money() : amount(0), currency("CNY") {}
    Money(int64_t a, const std::string& c = "CNY") : amount(a), currency(c) {}

    Money operator+(const Money& other) const {
        return Money(amount + other.amount, currency);
    }

    Money operator-(const Money& other) const {
        return Money(amount - other.amount, currency);
    }

    Money operator*(int factor) const {
        return Money(amount * factor, currency);
    }

    Money operator*(double factor) const {
        return Money(static_cast<int64_t>(amount * factor), currency);
    }

    bool operator==(const Money& other) const {
        return amount == other.amount && currency == other.currency;
    }

    bool operator<(const Money& other) const {
        return amount < other.amount;
    }

    bool operator>(const Money& other) const {
        return amount > other.amount;
    }

    bool operator<=(const Money& other) const {
        return amount <= other.amount;
    }

    bool operator>=(const Money& other) const {
        return amount >= other.amount;
    }

    bool operator!=(const Money& other) const {
        return !(*this == other);
    }

    double to_double() const {
        return static_cast<double>(amount) / 100.0;
    }

    static Money from_double(double value, const std::string& currency = "CNY") {
        return Money(static_cast<int64_t>(value * 100), currency);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Money& money) {
    os << money.to_double() << " " << money.currency;
    return os;
}

struct TimeRange {
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;

    bool contains(std::chrono::system_clock::time_point tp) const {
        return tp >= start && tp <= end;
    }
};

template<typename T>
struct PageResult {
    std::vector<T> items;
    size_t total;
    size_t page;
    size_t page_size;
    bool has_next() const { return (page + 1) * page_size < total; }
    bool has_prev() const { return page > 0; }
};

} // namespace oms

#endif // OMS_COMMON_TYPES_H
