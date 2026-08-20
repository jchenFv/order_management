#ifndef OMS_ORDER_ORDER_H
#define OMS_ORDER_ORDER_H

#include "common/types.h"
#include "common/result.h"
#include "order/order_item.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace oms {
namespace order {

class Order {
public:
    Order();
    Order(OrderId id, UserId user_id);

    OrderId id() const;
    void set_id(OrderId id);

    UserId user_id() const;
    UserRole user_role() const;
    void set_user_role(UserRole role);

    OrderStatus status() const;
    void set_status(OrderStatus status);
    Result can_transition_to(OrderStatus new_status) const;

    const std::string& order_number() const;
    void generate_order_number();

    const OrderItemCollection& items() const;
    OrderItemCollection& items();

    Result add_item(const OrderItem& item);
    Result remove_item(uint64_t item_id);
    Result clear_items();

    double subtotal() const;
    double total_discount() const;
    double shipping_fee() const;
    void set_shipping_fee(double fee);
    double tax_amount() const;
    void set_tax_rate(double rate);
    double total_amount() const;

    double paid_amount() const;
    void set_paid_amount(double amount);
    double remaining_amount() const;
    bool is_fully_paid() const;

    const std::string& shipping_address() const;
    void set_shipping_address(const std::string& address);
    const std::string& shipping_phone() const;
    void set_shipping_phone(const std::string& phone);
    const std::string& shipping_name() const;
    void set_shipping_name(const std::string& name);

    const std::string& billing_address() const;
    void set_billing_address(const std::string& address);

    PaymentId payment_id() const;
    void set_payment_id(PaymentId id);

    const std::vector<DiscountId>& applied_discounts() const;
    Result apply_discount(DiscountId discount_id);
    Result remove_discount(DiscountId discount_id);
    Result clear_discounts();
    bool has_discount(DiscountId discount_id) const;

    const std::string& coupon_code() const;
    void set_coupon_code(const std::string& code);

    int points_used() const;
    void set_points_used(int points);
    double points_discount() const;

    const std::string& remark() const;
    void set_remark(const std::string& remark);

    const std::map<std::string, std::string>& metadata() const;
    void set_metadata(const std::string& key, const std::string& value);
    std::string get_metadata(const std::string& key, const std::string& default_value = "") const;

    time_t created_at() const;
    time_t updated_at() const;
    time_t paid_at() const;
    void set_paid_at(time_t t);
    time_t shipped_at() const;
    void set_shipped_at(time_t t);
    time_t delivered_at() const;
    void set_delivered_at(time_t t);

    std::string status_to_string() const;

    Result cancel(const std::string& reason = "");
    Result mark_as_paid();
    Result mark_as_shipped(const std::string& tracking_number = "");
    Result mark_as_delivered();
    Result mark_as_refunded(double refund_amount);

    bool can_cancel() const;
    bool can_modify() const;
    bool can_apply_discount() const;

    const std::string& cancel_reason() const;
    const std::string& tracking_number() const;

    double refund_amount() const;
    bool is_refunded() const;
    bool is_partial_refund() const;

    WarehouseId warehouse_id() const;
    void set_warehouse_id(WarehouseId id);

    Result recalculate_totals();
    Result validate() const;

    std::string to_string() const;
    std::string to_json() const;

    bool operator==(const Order& other) const;
    bool operator!=(const Order& other) const;

private:
    OrderId id_;
    std::string order_number_;
    UserId user_id_;
    UserRole user_role_;
    OrderStatus status_;
    OrderItemCollection items_;
    double shipping_fee_;
    double tax_rate_;
    double paid_amount_;
    double refund_amount_;
    std::string shipping_address_;
    std::string shipping_phone_;
    std::string shipping_name_;
    std::string billing_address_;
    PaymentId payment_id_;
    std::vector<DiscountId> applied_discounts_;
    std::string coupon_code_;
    int points_used_;
    std::string remark_;
    std::map<std::string, std::string> metadata_;
    time_t created_at_;
    time_t updated_at_;
    time_t paid_at_;
    time_t shipped_at_;
    time_t delivered_at_;
    std::string cancel_reason_;
    std::string tracking_number_;
    WarehouseId warehouse_id_;
};

class OrderManager {
public:
    static OrderManager& instance();

    Result init();
    void shutdown();

    ResultT<OrderId> create_order(UserId user_id, const std::vector<OrderItem>& items,
                                    const std::string& shipping_address,
                                    const std::string& shipping_phone);

    ResultT<Order*> get_order(OrderId id);
    ResultT<std::vector<Order*>> get_user_orders(UserId user_id, size_t page = 0, size_t page_size = 20);
    ResultT<std::vector<Order*>> get_orders_by_status(OrderStatus status,
                                                        size_t page = 0, size_t page_size = 50);

    Result update_order_status(OrderId id, OrderStatus new_status, const std::string& reason = "");
    Result cancel_order(OrderId id, const std::string& reason = "");

    Result add_order_item(OrderId order_id, const OrderItem& item);
    Result remove_order_item(OrderId order_id, uint64_t item_id);
    Result update_item_quantity(OrderId order_id, uint64_t item_id, int new_quantity);

    Result apply_discount(OrderId order_id, DiscountId discount_id);
    Result apply_coupon(OrderId order_id, const std::string& coupon_code);
    Result remove_discount(OrderId order_id, DiscountId discount_id);

    Result update_shipping_info(OrderId order_id, const std::string& address,
                                 const std::string& phone, const std::string& name);

    Result set_payment_info(OrderId order_id, PaymentId payment_id, double amount);

    ResultT<size_t> get_order_count() const;
    ResultT<size_t> get_user_order_count(UserId user_id) const;
    ResultT<size_t> get_status_order_count(OrderStatus status) const;

    ResultT<double> get_total_sales(const TimeRange& range) const;
    ResultT<double> get_average_order_value(const TimeRange& range) const;

    ResultT<std::vector<Order*>> search_orders(const std::string& keyword,
                                                 size_t page = 0, size_t page_size = 50);

    ResultT<std::map<OrderStatus, size_t>> get_status_distribution() const;

    Result delete_order(OrderId id);
    Result archive_order(OrderId id);

    Result recalculate_order(OrderId id);
    Result validate_order(OrderId id);

private:
    OrderManager();
    ~OrderManager();

    Result reserve_inventory(Order* order);
    Result release_inventory(Order* order);

    std::map<OrderId, std::unique_ptr<Order>> orders_;
    std::map<UserId, std::vector<OrderId>> user_order_index_;
    std::map<OrderStatus, std::vector<OrderId>> status_index_;
    std::map<std::string, OrderId> order_number_index_;
    mutable std::shared_mutex mutex_;
    OrderId next_order_id_;
};

} // namespace order
} // namespace oms

#endif // OMS_ORDER_ORDER_H
