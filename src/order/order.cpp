#include "order/order.h"
#include "order/discount.h"
#include "inventory/warehouse.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace oms {
namespace order {

Order::Order()
    : id_(0), user_id_(0), user_role_(UserRole::GUEST), status_(OrderStatus::CREATED),
      shipping_fee_(0), tax_rate_(0.13), paid_amount_(0), refund_amount_(0),
      points_used_(0), created_at_(time(nullptr)), updated_at_(time(nullptr)),
      paid_at_(0), shipped_at_(0), delivered_at_(0), warehouse_id_(0) {
}

Order::Order(OrderId id, UserId user_id)
    : id_(id), user_id_(user_id), user_role_(UserRole::CUSTOMER), status_(OrderStatus::CREATED),
      shipping_fee_(0), tax_rate_(0.13), paid_amount_(0), refund_amount_(0),
      points_used_(0), created_at_(time(nullptr)), updated_at_(time(nullptr)),
      paid_at_(0), shipped_at_(0), delivered_at_(0), warehouse_id_(0) {
}

OrderId Order::id() const { return id_; }
void Order::set_id(OrderId id) { id_ = id; }

UserId Order::user_id() const { return user_id_; }
UserRole Order::user_role() const { return user_role_; }
void Order::set_user_role(UserRole role) { user_role_ = role; }

OrderStatus Order::status() const { return status_; }
void Order::set_status(OrderStatus status) { status_ = status; updated_at_ = time(nullptr); }

Result Order::can_transition_to(OrderStatus new_status) const {
    if (status_ == new_status) {
        return Result::ok();
    }

    if (status_ == OrderStatus::CANCELLED || status_ == OrderStatus::REFUNDED) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Cannot modify cancelled/refunded order");
    }

    switch (new_status) {
        case OrderStatus::PAID:
            if (status_ != OrderStatus::CREATED && status_ != OrderStatus::PENDING_PAYMENT) {
                return Result::error(ErrorCode::INVALID_PARAMETER, "Invalid state transition");
            }
            break;
        case OrderStatus::SHIPPED:
            if (status_ != OrderStatus::PAID && status_ != OrderStatus::PROCESSING) {
                return Result::error(ErrorCode::INVALID_PARAMETER, "Order must be paid first");
            }
            break;
        case OrderStatus::CANCELLED:
            if (status_ == OrderStatus::DELIVERED || status_ == OrderStatus::SHIPPED) {
                return Result::error(ErrorCode::INVALID_PARAMETER, "Cannot cancel shipped/delivered order");
            }
            break;
        default:
            break;
    }

    return Result::ok();
}

const std::string& Order::order_number() const { return order_number_; }

void Order::generate_order_number() {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);

    std::ostringstream oss;
    oss << "ORD"
        << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
        << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
        << std::setw(2) << std::setfill('0') << tm.tm_mday
        << "-"
        << std::setw(8) << std::setfill('0') << id_;

    order_number_ = oss.str();
}

const OrderItemCollection& Order::items() const { return items_; }
OrderItemCollection& Order::items() { return items_; }

Result Order::add_item(const OrderItem& item) {
    return items_.add_item(item);
}

Result Order::remove_item(uint64_t item_id) {
    return items_.remove_item(item_id);
}

Result Order::clear_items() {
    items_.clear();
    return Result::ok();
}

double Order::subtotal() const {
    double total = 0;
    for (const auto& item : items_.items()) {
        total += item.unit_price() * item.quantity();
    }
    return total;
}

double Order::total_discount() const {
    double total = 0;
    for (const auto& item : items_.items()) {
        total += item.discount_amount();
    }
    return total;
}

double Order::shipping_fee() const { return shipping_fee_; }
void Order::set_shipping_fee(double fee) { shipping_fee_ = fee; }

double Order::tax_amount() const {
    double discount_rate = total_discount() / subtotal();
    return (subtotal() * (1.0 - discount_rate) + shipping_fee_) * tax_rate_;
}

void Order::set_tax_rate(double rate) { tax_rate_ = rate; }

double Order::total_amount() const {
    return subtotal() - total_discount() + shipping_fee() + tax_amount();
}

double Order::paid_amount() const { return paid_amount_; }
void Order::set_paid_amount(double amount) { paid_amount_ = amount; }

double Order::remaining_amount() const {
    return total_amount() - paid_amount_;
}

bool Order::is_fully_paid() const {
    return remaining_amount() <= 0.01;
}

const std::string& Order::shipping_address() const { return shipping_address_; }
void Order::set_shipping_address(const std::string& address) { shipping_address_ = address; }

const std::string& Order::shipping_phone() const { return shipping_phone_; }
void Order::set_shipping_phone(const std::string& phone) { shipping_phone_ = phone; }

const std::string& Order::shipping_name() const { return shipping_name_; }
void Order::set_shipping_name(const std::string& name) { shipping_name_ = name; }

const std::string& Order::billing_address() const { return billing_address_; }
void Order::set_billing_address(const std::string& address) { billing_address_ = address; }

PaymentId Order::payment_id() const { return payment_id_; }
void Order::set_payment_id(PaymentId id) { payment_id_ = id; }

const std::vector<DiscountId>& Order::applied_discounts() const { return applied_discounts_; }

Result Order::apply_discount(DiscountId discount_id) {
    for (auto id : applied_discounts_) {
        if (id == discount_id) {
            return Result::error(ErrorCode::INVALID_PARAMETER, "Discount already applied");
        }
    }

    auto& dm = DiscountManager::instance();
    auto result = dm.get_discount(discount_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    auto discount = result.value();
    if (!discount->is_active()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Discount is not active");
    }

    if (!discount->is_applicable(*this)) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Discount not applicable");
    }

    double discount_value = discount->calculate_discount(*this);
    if (discount_value == 0.0) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Discount value is zero");
    }

    applied_discounts_.push_back(discount_id);
    discount->increment_usage();

    return Result::ok();
}

Result Order::remove_discount(DiscountId discount_id) {
    auto it = std::find(applied_discounts_.begin(), applied_discounts_.end(), discount_id);
    if (it == applied_discounts_.end()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Discount not found");
    }

    applied_discounts_.erase(it);
    return Result::ok();
}

Result Order::clear_discounts() {
    applied_discounts_.clear();
    return Result::ok();
}

bool Order::has_discount(DiscountId discount_id) const {
    return std::find(applied_discounts_.begin(), applied_discounts_.end(), discount_id)
           != applied_discounts_.end();
}

const std::string& Order::coupon_code() const { return coupon_code_; }
void Order::set_coupon_code(const std::string& code) { coupon_code_ = code; }

int Order::points_used() const { return points_used_; }
void Order::set_points_used(int points) { points_used_ = points; }

double Order::points_discount() const {
    return static_cast<double>(points_used_) / 100.0;
}

const std::string& Order::remark() const { return remark_; }
void Order::set_remark(const std::string& remark) { remark_ = remark; }

const std::map<std::string, std::string>& Order::metadata() const { return metadata_; }

void Order::set_metadata(const std::string& key, const std::string& value) {
    metadata_[key] = value;
}

std::string Order::get_metadata(const std::string& key, const std::string& default_value) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second;
    }
    return default_value;
}

time_t Order::created_at() const { return created_at_; }
time_t Order::updated_at() const { return updated_at_; }
time_t Order::paid_at() const { return paid_at_; }
void Order::set_paid_at(time_t t) { paid_at_ = t; }
time_t Order::shipped_at() const { return shipped_at_; }
void Order::set_shipped_at(time_t t) { shipped_at_ = t; }
time_t Order::delivered_at() const { return delivered_at_; }
void Order::set_delivered_at(time_t t) { delivered_at_ = t; }

std::string Order::status_to_string() const {
    switch (status_) {
        case OrderStatus::CREATED: return "CREATED";
        case OrderStatus::PENDING_PAYMENT: return "PENDING_PAYMENT";
        case OrderStatus::PAID: return "PAID";
        case OrderStatus::PROCESSING: return "PROCESSING";
        case OrderStatus::SHIPPED: return "SHIPPED";
        case OrderStatus::DELIVERED: return "DELIVERED";
        case OrderStatus::COMPLETED: return "COMPLETED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REFUNDED: return "REFUNDED";
        default: return "UNKNOWN";
    }
}

Result Order::cancel(const std::string& reason) {
    Result transition = can_transition_to(OrderStatus::CANCELLED);
    if (!transition) {
        return transition;
    }

    cancel_reason_ = reason;
    set_status(OrderStatus::CANCELLED);

    return Result::ok();
}

Result Order::mark_as_paid() {
    Result transition = can_transition_to(OrderStatus::PAID);
    if (!transition) {
        return transition;
    }

    set_status(OrderStatus::PAID);
    paid_at_ = time(nullptr);
    return Result::ok();
}

Result Order::mark_as_shipped(const std::string& tracking_number) {
    Result transition = can_transition_to(OrderStatus::SHIPPED);
    if (!transition) {
        return transition;
    }

    tracking_number_ = tracking_number;
    set_status(OrderStatus::SHIPPED);
    shipped_at_ = time(nullptr);
    return Result::ok();
}

Result Order::mark_as_delivered() {
    Result transition = can_transition_to(OrderStatus::DELIVERED);
    if (!transition) {
        return transition;
    }

    set_status(OrderStatus::DELIVERED);
    delivered_at_ = time(nullptr);
    return Result::ok();
}

Result Order::mark_as_refunded(double refund_amount) {
    Result transition = can_transition_to(OrderStatus::REFUNDED);
    if (!transition) {
        return transition;
    }

    refund_amount_ = refund_amount;
    set_status(OrderStatus::REFUNDED);
    return Result::ok();
}

bool Order::can_cancel() const {
    return status_ == OrderStatus::CREATED ||
           status_ == OrderStatus::PENDING_PAYMENT;
}

bool Order::can_modify() const {
    return status_ == OrderStatus::CREATED;
}

bool Order::can_apply_discount() const {
    return status_ == OrderStatus::CREATED ||
           status_ == OrderStatus::PENDING_PAYMENT;
}

const std::string& Order::cancel_reason() const { return cancel_reason_; }
const std::string& Order::tracking_number() const { return tracking_number_; }

double Order::refund_amount() const { return refund_amount_; }
bool Order::is_refunded() const { return status_ == OrderStatus::REFUNDED; }
bool Order::is_partial_refund() const { return refund_amount_ > 0 && refund_amount_ < paid_amount_; }

WarehouseId Order::warehouse_id() const { return warehouse_id_; }
void Order::set_warehouse_id(WarehouseId id) { warehouse_id_ = id; }

Result Order::recalculate_totals() {
    return Result::ok();
}

Result Order::validate() const {
    if (items_.size() == 0) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Order has no items");
    }

    if (shipping_address_.empty()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Shipping address required");
    }

    if (shipping_phone_.empty()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Shipping phone required");
    }

    if (total_amount() <= 0) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Order total must be positive");
    }

    return Result::ok();
}

std::string Order::to_string() const {
    std::ostringstream oss;
    oss << "Order(id=" << id_
        << ", user=" << user_id_
        << ", status=" << status_to_string()
        << ", items=" << items_.size()
        << ", total=" << total_amount()
        << ")";
    return oss.str();
}

std::string Order::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"id\":" << id_ << ",";
    oss << "\"user_id\":" << user_id_ << ",";
    oss << "\"status\":\"" << status_to_string() << "\",";
    oss << "\"item_count\":" << items_.size() << ",";
    oss << "\"total_amount\":" << total_amount();
    oss << "}";
    return oss.str();
}

bool Order::operator==(const Order& other) const { return id_ == other.id_; }
bool Order::operator!=(const Order& other) const { return !(*this == other); }

OrderManager& OrderManager::instance() {
    static OrderManager instance;
    return instance;
}

OrderManager::OrderManager() : next_order_id_(1) {
}

OrderManager::~OrderManager() {
}

Result OrderManager::init() {
    return Result::ok();
}

void OrderManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    orders_.clear();
    user_order_index_.clear();
    status_index_.clear();
    order_number_index_.clear();
}

ResultT<OrderId> OrderManager::create_order(UserId user_id,
                                              const std::vector<OrderItem>& items,
                                              const std::string& shipping_address,
                                              const std::string& shipping_phone) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    OrderId id = next_order_id_++;
    auto order = std::make_unique<Order>(id, user_id);

    for (const auto& item : items) {
        order->add_item(item);
    }

    order->set_shipping_address(shipping_address);
    order->set_shipping_phone(shipping_phone);
    order->generate_order_number();

    auto order_ptr = order.get();
    orders_[id] = std::move(order);
    user_order_index_[user_id].push_back(id);
    status_index_[OrderStatus::CREATED].push_back(id);

    return ResultT<OrderId>::ok(id);
}

ResultT<Order*> OrderManager::get_order(OrderId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return ResultT<Order*>::error(ErrorCode::ORDER_NOT_FOUND, "Order not found");
    }

    return ResultT<Order*>::ok(it->second.get());
}

ResultT<std::vector<Order*>> OrderManager::get_user_orders(UserId user_id,
                                                              size_t page, size_t page_size) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Order*> result;
    auto it = user_order_index_.find(user_id);
    if (it == user_order_index_.end()) {
        return ResultT<std::vector<Order*>>::ok(result);
    }

    const auto& order_ids = it->second;
    size_t start = page * page_size;
    size_t end = std::min(start + page_size, order_ids.size());

    for (size_t i = start; i < end; i++) {
        size_t reversed_idx = order_ids.size() - 1 - i;
        if (reversed_idx >= order_ids.size()) break;

        auto order_it = orders_.find(order_ids[reversed_idx]);
        if (order_it != orders_.end()) {
            result.push_back(order_it->second.get());
        }
    }

    return ResultT<std::vector<Order*>>::ok(result);
}

ResultT<std::vector<Order*>> OrderManager::get_orders_by_status(OrderStatus status,
                                                                  size_t page, size_t page_size) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Order*> result;
    auto it = status_index_.find(status);
    if (it == status_index_.end()) {
        return ResultT<std::vector<Order*>>::ok(result);
    }

    const auto& order_ids = it->second;
    size_t start = page * page_size;
    size_t end = std::min(start + page_size, order_ids.size());

    for (size_t i = start; i < end; i++) {
        auto order_it = orders_.find(order_ids[i]);
        if (order_it != orders_.end()) {
            result.push_back(order_it->second.get());
        }
    }

    return ResultT<std::vector<Order*>>::ok(result);
}

Result OrderManager::update_order_status(OrderId id, OrderStatus new_status,
                                          const std::string& reason) {
    auto result = get_order(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    Result r = order->can_transition_to(new_status);
    if (!r) {
        return r;
    }

    OrderStatus old_status = order->status();
    order->set_status(new_status);

    auto& old_list = status_index_[old_status];
    old_list.erase(std::remove(old_list.begin(), old_list.end(), id), old_list.end());
    status_index_[new_status].push_back(id);

    return Result::ok();
}

Result OrderManager::cancel_order(OrderId id, const std::string& reason) {
    auto result = get_order(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    return result.value()->cancel(reason);
}

Result OrderManager::add_order_item(OrderId order_id, const OrderItem& item) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    if (!order->can_modify()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Order cannot be modified");
    }

    return order->add_item(item);
}

Result OrderManager::remove_order_item(OrderId order_id, uint64_t item_id) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    if (!order->can_modify()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Order cannot be modified");
    }

    return order->remove_item(item_id);
}

Result OrderManager::update_item_quantity(OrderId order_id, uint64_t item_id, int new_quantity) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    return result.value()->items().update_quantity(item_id, new_quantity);
}

Result OrderManager::apply_discount(OrderId order_id, DiscountId discount_id) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    if (!order->can_apply_discount()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Cannot apply discount to this order");
    }

    return order->apply_discount(discount_id);
}

Result OrderManager::apply_coupon(OrderId order_id, const std::string& coupon_code) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    order->set_coupon_code(coupon_code);
    return Result::ok();
}

Result OrderManager::remove_discount(OrderId order_id, DiscountId discount_id) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    return result.value()->remove_discount(discount_id);
}

Result OrderManager::update_shipping_info(OrderId order_id, const std::string& address,
                                            const std::string& phone, const std::string& name) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    if (!order->can_modify()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Order cannot be modified");
    }

    order->set_shipping_address(address);
    order->set_shipping_phone(phone);
    order->set_shipping_name(name);
    return Result::ok();
}

Result OrderManager::set_payment_info(OrderId order_id, PaymentId payment_id, double amount) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto order = result.value();
    order->set_payment_id(payment_id);
    order->set_paid_amount(amount);
    return Result::ok();
}

ResultT<size_t> OrderManager::get_order_count() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return ResultT<size_t>::ok(orders_.size());
}

ResultT<size_t> OrderManager::get_user_order_count(UserId user_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = user_order_index_.find(user_id);
    if (it == user_order_index_.end()) {
        return ResultT<size_t>::ok(0);
    }
    return ResultT<size_t>::ok(it->second.size());
}

ResultT<size_t> OrderManager::get_status_order_count(OrderStatus status) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = status_index_.find(status);
    if (it == status_index_.end()) {
        return ResultT<size_t>::ok(0);
    }
    return ResultT<size_t>::ok(it->second.size());
}

ResultT<double> OrderManager::get_total_sales(const TimeRange& range) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    double total = 0;
    for (const auto& pair : orders_) {
        auto& order = pair.second;
        auto order_time = std::chrono::system_clock::from_time_t(order->created_at());
        if (order_time >= range.start && order_time <= range.end &&
            (order->status() == OrderStatus::PAID ||
             order->status() == OrderStatus::SHIPPED ||
             order->status() == OrderStatus::DELIVERED ||
             order->status() == OrderStatus::COMPLETED)) {
            total += order->total_amount();
        }
    }

    return ResultT<double>::ok(total);
}

ResultT<double> OrderManager::get_average_order_value(const TimeRange& range) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    double total = 0;
    int count = 0;

    for (const auto& pair : orders_) {
        auto& order = pair.second;
        auto order_time = std::chrono::system_clock::from_time_t(order->created_at());
        if (order_time >= range.start && order_time <= range.end &&
            (order->status() == OrderStatus::PAID ||
             order->status() == OrderStatus::SHIPPED ||
             order->status() == OrderStatus::DELIVERED ||
             order->status() == OrderStatus::COMPLETED)) {
            total += order->total_amount();
            count++;
        }
    }

    double avg = count > 0 ? total / count : 0;
    return ResultT<double>::ok(avg);
}

ResultT<std::vector<Order*>> OrderManager::search_orders(const std::string& keyword,
                                                           size_t page, size_t page_size) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Order*> all_matches;
    std::string lower_keyword = keyword;
    std::transform(lower_keyword.begin(), lower_keyword.end(),
                   lower_keyword.begin(), ::tolower);

    for (const auto& pair : orders_) {
        auto& order = pair.second;
        std::string order_num = order->order_number();
        std::transform(order_num.begin(), order_num.end(), order_num.begin(), ::tolower);

        if (order_num.find(lower_keyword) != std::string::npos ||
            order->shipping_address().find(keyword) != std::string::npos ||
            order->remark().find(keyword) != std::string::npos) {
            all_matches.push_back(order.get());
        }
    }

    std::vector<Order*> result;
    size_t start = page * page_size;
    size_t end = std::min(start + page_size, all_matches.size());

    for (size_t i = start; i < end; i++) {
        result.push_back(all_matches[i]);
    }

    return ResultT<std::vector<Order*>>::ok(result);
}

ResultT<std::map<OrderStatus, size_t>> OrderManager::get_status_distribution() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::map<OrderStatus, size_t> distribution;
    for (const auto& pair : status_index_) {
        distribution[pair.first] = pair.second.size();
    }

    return ResultT<std::map<OrderStatus, size_t>>::ok(distribution);
}

Result OrderManager::delete_order(OrderId id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return Result::error(ErrorCode::ORDER_NOT_FOUND, "Order not found");
    }

    UserId user_id = it->second->user_id();
    OrderStatus status = it->second->status();

    auto& user_orders = user_order_index_[user_id];
    user_orders.erase(std::remove(user_orders.begin(), user_orders.end(), id), user_orders.end());

    auto& status_orders = status_index_[status];
    status_orders.erase(std::remove(status_orders.begin(), status_orders.end(), id), status_orders.end());

    orders_.erase(it);
    return Result::ok();
}

Result OrderManager::archive_order(OrderId id) {
    return Result::ok();
}

Result OrderManager::recalculate_order(OrderId id) {
    auto result = get_order(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    return result.value()->recalculate_totals();
}

Result OrderManager::validate_order(OrderId id) {
    auto result = get_order(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    return result.value()->validate();
}

Result OrderManager::reserve_inventory(Order* order) {
    (void)order;
    return Result::ok();
}

Result OrderManager::release_inventory(Order* order) {
    (void)order;
    return Result::ok();
}

OrderItem OrderManager::parse_csv_item(const std::string& line) {
    char buf[256];
    strcpy(buf, line.c_str());

    char* token = strtok(buf, ",");
    ProductId product_id = atoll(token);

    token = strtok(nullptr, ",");
    std::string sku = token ? token : "";

    token = strtok(nullptr, ",");
    int quantity = atoi(token);

    token = strtok(nullptr, ",");
    double price = atof(token);

    OrderItem item(product_id, sku, quantity, price);
    return item;
}

void OrderManager::process_csv_buffer(const char* buffer, size_t len, std::vector<OrderItem>& items) {
    std::string line;
    for (size_t i = 0; i < len; i++) {
        if (buffer[i] == '\n') {
            if (!line.empty()) {
                items.push_back(parse_csv_item(line));
            }
            line.clear();
        } else {
            line += buffer[i];
        }
    }
}

ResultT<size_t> OrderManager::import_orders_from_csv(const std::string& csv_content, UserId created_by) {
    std::vector<OrderItem> items;
    process_csv_buffer(csv_content.c_str(), csv_content.size(), items);

    std::string address;
    std::string phone;

    size_t imported = 0;
    for (size_t i = 0; i < items.size(); i += 5) {
        std::vector<OrderItem> order_items;
        for (size_t j = 0; j < 5 && i + j < items.size(); j++) {
            order_items.push_back(items[i + j]);
        }

        auto result = create_order(created_by, order_items, address, phone);
        if (result) {
            imported++;
        }
    }

    return ResultT<size_t>::ok(imported);
}

std::string OrderManager::order_to_csv_line(const Order& order) {
    char buf[1024];
    sprintf(buf, "%lu,%lu,%.2f,%s\n",
            order.id(),
            order.user_id(),
            order.total_amount(),
            order.shipping_address().c_str());
    return std::string(buf);
}

ResultT<std::string> OrderManager::export_orders_to_csv(const std::vector<OrderId>& order_ids) {
    std::string csv = "order_id,user_id,total_amount,address\n";

    for (size_t i = 0; i < order_ids.size(); i++) {
        auto result = get_order(order_ids[i]);
        if (result) {
            csv += order_to_csv_line(*result.value());
        }
    }

    return ResultT<std::string>::ok(csv);
}

Result OrderManager::batch_update_status(const std::vector<OrderId>& order_ids, OrderStatus new_status) {
    for (size_t i = 0; i < order_ids.size(); i++) {
        update_order_status(order_ids[i], new_status);
    }
    return Result::ok();
}

Result OrderManager::batch_apply_discount(const std::vector<OrderId>& order_ids, DiscountId discount_id) {
    auto& dm = DiscountManager::instance();
    auto discount_result = dm.get_discount(discount_id);
    if (!discount_result) {
        return Result::error(discount_result.error_code(), discount_result.error_message());
    }

    int usage_count = discount_result.value()->usage_count();
    int max_usage = discount_result.value()->usage_limit();

    if (max_usage > 0) {
        for (size_t i = 0; i < order_ids.size(); i++) {
            if (usage_count >= max_usage) {
                break;
            }
            Result r = apply_discount(order_ids[i], discount_id);
            if (r) {
                usage_count++;
            }
        }
    }

    return Result::ok();
}

ResultT<std::vector<Order*>> OrderManager::get_orders_by_amount_range(double min_amount, double max_amount) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<Order*> result;

    for (const auto& pair : orders_) {
        double diff = pair.second->total_amount() - min_amount;
        if (diff >= 0 && pair.second->total_amount() <= max_amount) {
            result.push_back(pair.second.get());
        }
    }

    return ResultT<std::vector<Order*>>::ok(result);
}

Result OrderManager::recalculate_shipping_fee(OrderId order_id) {
    auto result = get_order(order_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    Order* order = result.value();
    double weight = 0;
    for (const auto& item : order->items().items()) {
        weight += item.quantity() * 0.5;
    }

    auto& wh = inventory::WarehouseManager::instance();
    auto wh_result = wh.get_default_warehouse();
    double distance = 100.0;
    if (wh_result) {
        distance = wh_result.value()->distance_to(0.0, 0.0);
    }

    double fee = weight * distance / 100.0;
    order->set_shipping_fee(fee);

    return order->recalculate_totals();
}

} // namespace order
} // namespace oms
