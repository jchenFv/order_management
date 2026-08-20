#include "inventory/warehouse.h"
#include <algorithm>
#include <cstring>

namespace oms {
namespace inventory {

Warehouse::Warehouse()
    : id_(0), priority_(0), is_active_(true), is_default_(false),
      shipping_cost_multiplier_(1.0), created_at_(time(nullptr)),
      updated_at_(time(nullptr)) {
    memset(&capabilities_, 0, sizeof(capabilities_));
    capabilities_.can_ship_domestic = true;
}

Warehouse::Warehouse(WarehouseId id, const std::string& name, const std::string& code)
    : id_(id), name_(name), code_(code), priority_(0), is_active_(true),
      is_default_(false), shipping_cost_multiplier_(1.0),
      created_at_(time(nullptr)), updated_at_(time(nullptr)) {
    memset(&capabilities_, 0, sizeof(capabilities_));
    capabilities_.can_ship_domestic = true;
}

WarehouseId Warehouse::id() const {
    return id_;
}

const std::string& Warehouse::name() const {
    return name_;
}

void Warehouse::set_name(const std::string& name) {
    name_ = name;
    update_timestamp();
}

const std::string& Warehouse::code() const {
    return code_;
}

void Warehouse::set_code(const std::string& code) {
    code_ = code;
    update_timestamp();
}

const WarehouseAddress& Warehouse::address() const {
    return address_;
}

WarehouseAddress& Warehouse::address() {
    return address_;
}

void Warehouse::set_address(const WarehouseAddress& addr) {
    address_ = addr;
    update_timestamp();
}

const WarehouseContact& Warehouse::contact() const {
    return contact_;
}

WarehouseContact& Warehouse::contact() {
    return contact_;
}

void Warehouse::set_contact(const WarehouseContact& contact) {
    contact_ = contact;
    update_timestamp();
}

const WarehouseCapabilities& Warehouse::capabilities() const {
    return capabilities_;
}

WarehouseCapabilities& Warehouse::capabilities() {
    return capabilities_;
}

void Warehouse::set_capabilities(const WarehouseCapabilities& caps) {
    capabilities_ = caps;
    update_timestamp();
}

int Warehouse::priority() const {
    return priority_;
}

void Warehouse::set_priority(int priority) {
    priority_ = priority;
    update_timestamp();
}

bool Warehouse::is_active() const {
    return is_active_;
}

void Warehouse::set_active(bool active) {
    is_active_ = active;
    update_timestamp();
}

bool Warehouse::is_default() const {
    return is_default_;
}

void Warehouse::set_default(bool is_default) {
    is_default_ = is_default;
    update_timestamp();
}

double Warehouse::shipping_cost_multiplier() const {
    return shipping_cost_multiplier_;
}

void Warehouse::set_shipping_cost_multiplier(double multiplier) {
    shipping_cost_multiplier_ = multiplier;
    update_timestamp();
}

int Warehouse::get_product_stock(ProductId product_id) const {
    auto it = stock_.find(product_id);
    if (it != stock_.end()) {
        return it->second;
    }
    return 0;
}

void Warehouse::set_product_stock(ProductId product_id, int quantity) {
    stock_[product_id] = quantity;
    update_timestamp();
}

int Warehouse::adjust_product_stock(ProductId product_id, int delta) {
    stock_[product_id] += delta;
    update_timestamp();
    return stock_[product_id];
}

bool Warehouse::has_product(ProductId product_id) const {
    return stock_.count(product_id) > 0 && stock_.at(product_id) > 0;
}

bool Warehouse::has_available_stock(ProductId product_id, int quantity) const {
    int available = get_product_stock(product_id) - get_reserved_stock(product_id);
    return available >= quantity;
}

const std::map<ProductId, int>& Warehouse::all_stock() const {
    return stock_;
}

int Warehouse::get_reserved_stock(ProductId product_id) const {
    auto it = reserved_stock_.find(product_id);
    if (it != reserved_stock_.end()) {
        return it->second;
    }
    return 0;
}

void Warehouse::set_reserved_stock(ProductId product_id, int quantity) {
    reserved_stock_[product_id] = quantity;
}

Result Warehouse::reserve_stock(ProductId product_id, int quantity) {
    if (!has_available_stock(product_id, quantity)) {
        return Result::error(ErrorCode::PRODUCT_OUT_OF_STOCK, "Insufficient stock");
    }
    reserved_stock_[product_id] += quantity;
    return Result::ok();
}

Result Warehouse::release_stock(ProductId product_id, int quantity) {
    if (reserved_stock_[product_id] < quantity) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Not enough reserved stock");
    }
    reserved_stock_[product_id] -= quantity;
    stock_[product_id] -= quantity;
    return Result::ok();
}

Result Warehouse::confirm_deduction(ProductId product_id, int quantity) {
    auto result = release_stock(product_id, quantity);
    if (!result) {
        return result;
    }
    stock_[product_id] -= quantity;
    update_timestamp();
    return Result::ok();
}

double Warehouse::distance_to(double lat, double lng) const {
    double lat_diff = lat - address_.latitude;
    double lng_diff = lng - address_.longitude;
    return std::sqrt(lat_diff * lat_diff + lng_diff * lng_diff);
}

time_t Warehouse::created_at() const {
    return created_at_;
}

time_t Warehouse::updated_at() const {
    return updated_at_;
}

void Warehouse::update_timestamp() {
    updated_at_ = time(nullptr);
}

std::string Warehouse::to_string() const {
    return name_ + " (" + code_ + ")";
}

std::string Warehouse::to_json() const {
    return "{\"id\":" + std::to_string(id_) + ",\"name\":\"" + name_ + "\",\"code\":\"" + code_ + "\"}";
}

WarehouseManager& WarehouseManager::instance() {
    static WarehouseManager instance;
    return instance;
}

WarehouseManager::WarehouseManager()
    : default_warehouse_id_(0) {
}

WarehouseManager::~WarehouseManager() {
}

Result WarehouseManager::init() {
    return Result::ok();
}

void WarehouseManager::shutdown() {
    warehouses_.clear();
    code_index_.clear();
    allocations_.clear();
}

ResultT<WarehouseId> WarehouseManager::create_warehouse(const std::string& name, const std::string& code) {
    if (code_index_.count(code) > 0) {
        return ResultT<WarehouseId>::error(ErrorCode::INVALID_PARAMETER, "Warehouse code already exists");
    }

    WarehouseId id = warehouses_.size() + 1;
    auto warehouse = std::make_unique<Warehouse>(id, name, code);
    warehouses_[id] = std::move(warehouse);
    code_index_[code] = id;

    return ResultT<WarehouseId>::ok(id);
}

Result WarehouseManager::delete_warehouse(WarehouseId id) {
    auto it = warehouses_.find(id);
    if (it == warehouses_.end()) {
        return Result::error(ErrorCode::WAREHOUSE_NOT_FOUND, "Warehouse not found");
    }

    code_index_.erase(it->second->code());
    warehouses_.erase(it);
    return Result::ok();
}

ResultT<Warehouse*> WarehouseManager::get_warehouse(WarehouseId id) {
    auto it = warehouses_.find(id);
    if (it == warehouses_.end()) {
        return ResultT<Warehouse*>::error(ErrorCode::WAREHOUSE_NOT_FOUND, "Warehouse not found");
    }
    return ResultT<Warehouse*>::ok(it->second.get());
}

ResultT<Warehouse*> WarehouseManager::get_warehouse_by_code(const std::string& code) {
    auto it = code_index_.find(code);
    if (it == code_index_.end()) {
        return ResultT<Warehouse*>::error(ErrorCode::WAREHOUSE_NOT_FOUND, "Warehouse not found");
    }
    return get_warehouse(it->second);
}

ResultT<Warehouse*> WarehouseManager::get_default_warehouse() {
    if (default_warehouse_id_ == 0) {
        return ResultT<Warehouse*>::error(ErrorCode::WAREHOUSE_NOT_FOUND, "No default warehouse");
    }
    return get_warehouse(default_warehouse_id_);
}

ResultT<std::vector<Warehouse*>> WarehouseManager::list_warehouses() {
    std::vector<Warehouse*> result;
    for (const auto& pair : warehouses_) {
        result.push_back(pair.second.get());
    }
    return ResultT<std::vector<Warehouse*>>::ok(result);
}

ResultT<std::vector<Warehouse*>> WarehouseManager::list_active_warehouses() {
    std::vector<Warehouse*> result;
    for (const auto& pair : warehouses_) {
        if (pair.second->is_active()) {
            result.push_back(pair.second.get());
        }
    }
    return ResultT<std::vector<Warehouse*>>::ok(result);
}

Result WarehouseManager::activate_warehouse(WarehouseId id) {
    auto result = get_warehouse(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->set_active(true);
    return Result::ok();
}

Result WarehouseManager::deactivate_warehouse(WarehouseId id) {
    auto result = get_warehouse(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->set_active(false);
    return Result::ok();
}

Result WarehouseManager::set_default_warehouse(WarehouseId id) {
    auto result = get_warehouse(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    default_warehouse_id_ = id;
    return Result::ok();
}

ResultT<std::vector<Warehouse*>> WarehouseManager::find_warehouses_with_product(ProductId product_id, int min_quantity) {
    std::vector<Warehouse*> result;
    for (const auto& pair : warehouses_) {
        if (pair.second->is_active() && pair.second->has_available_stock(product_id, min_quantity)) {
            result.push_back(pair.second.get());
        }
    }
    return ResultT<std::vector<Warehouse*>>::ok(result);
}

ResultT<Warehouse*> WarehouseManager::find_nearest_warehouse(double lat, double lng, ProductId product_id) {
    Warehouse* nearest = nullptr;
    double min_distance = 1e18;

    for (const auto& pair : warehouses_) {
        Warehouse* wh = pair.second.get();
        if (!wh->is_active()) {
            continue;
        }
        if (product_id != 0 && !wh->has_product(product_id)) {
            continue;
        }

        double dist = wh->distance_to(lat, lng);
        if (dist < min_distance) {
            min_distance = dist;
            nearest = wh;
        }
    }

    if (!nearest) {
        return ResultT<Warehouse*>::error(ErrorCode::WAREHOUSE_NOT_FOUND, "No suitable warehouse found");
    }
    return ResultT<Warehouse*>::ok(nearest);
}

ResultT<std::map<WarehouseId, int>> WarehouseManager::get_product_distribution(ProductId product_id) {
    std::map<WarehouseId, int> result;
    for (const auto& pair : warehouses_) {
        int stock = pair.second->get_product_stock(product_id);
        if (stock > 0) {
            result[pair.first] = stock;
        }
    }
    return ResultT<std::map<WarehouseId, int>>::ok(result);
}

ResultT<int> WarehouseManager::get_total_product_stock(ProductId product_id) {
    int total = 0;
    for (const auto& pair : warehouses_) {
        total += pair.second->get_product_stock(product_id);
    }
    return ResultT<int>::ok(total);
}

Result WarehouseManager::transfer_stock(ProductId product_id, WarehouseId from_warehouse, WarehouseId to_warehouse, int quantity, const std::string& reason) {
    auto from_result = get_warehouse(from_warehouse);
    auto to_result = get_warehouse(to_warehouse);

    if (!from_result) {
        return Result::error(from_result.error_code(), from_result.error_message());
    }
    if (!to_result) {
        return Result::error(to_result.error_code(), to_result.error_message());
    }

    Warehouse* from = from_result.value();
    Warehouse* to = to_result.value();

    if (!from->has_available_stock(product_id, quantity)) {
        return Result::error(ErrorCode::PRODUCT_OUT_OF_STOCK, "Insufficient stock in source warehouse");
    }

    from->adjust_product_stock(product_id, -quantity);
    to->adjust_product_stock(product_id, quantity);

    return Result::ok();
}

Result WarehouseManager::receive_stock(ProductId product_id, WarehouseId warehouse_id, int quantity, const std::string& reason) {
    auto result = get_warehouse(warehouse_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->adjust_product_stock(product_id, quantity);
    return Result::ok();
}

Result WarehouseManager::adjust_inventory(ProductId product_id, WarehouseId warehouse_id, int quantity, const std::string& reason) {
    auto result = get_warehouse(warehouse_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->adjust_product_stock(product_id, quantity);
    return Result::ok();
}

ResultT<std::pair<WarehouseId, int>> WarehouseManager::allocate_order(
    OrderId order_id,
    const std::vector<std::pair<ProductId, int>>& items,
    const std::string& shipping_address) {

    std::map<WarehouseId, int> warehouse_counts;
    WarehouseId selected = 0;

    for (const auto& item : items) {
        ProductId product_id = item.first;
        int quantity = item.second;

        auto wh_result = find_warehouses_with_product(product_id, quantity);
        if (!wh_result || wh_result.value().empty()) {
            return ResultT<std::pair<WarehouseId, int>>::error(ErrorCode::PRODUCT_OUT_OF_STOCK, "No warehouse has sufficient stock");
        }

        for (Warehouse* wh : wh_result.value()) {
            warehouse_counts[wh->id()]++;
        }
    }

    int max_count = 0;
    for (const auto& pair : warehouse_counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            selected = pair.first;
        }
    }

    if (selected == 0 && !warehouses_.empty()) {
        selected = warehouses_.begin()->first;
    }

    return ResultT<std::pair<WarehouseId, int>>::ok(std::make_pair(selected, max_count));
}

Result WarehouseManager::release_allocation(OrderId order_id) {
    allocations_.erase(order_id);
    return Result::ok();
}

} // namespace inventory
} // namespace oms
