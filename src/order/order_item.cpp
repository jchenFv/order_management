#include "order/order_item.h"
#include <algorithm>

namespace oms {
namespace order {

OrderItem::OrderItem()
    : id_(0), product_id_(0), quantity_(1), unit_price_(0.0), discount_amount_(0.0),
      warehouse_id_(0), is_gift_(false), has_snapshot_(false) {
}

OrderItem::OrderItem(ProductId product_id, const std::string& name, int quantity, double unit_price)
    : id_(0), product_id_(product_id), product_name_(name), quantity_(quantity),
      unit_price_(unit_price), discount_amount_(0.0), warehouse_id_(0),
      is_gift_(false), has_snapshot_(false) {
}

uint64_t OrderItem::id() const {
    return id_;
}

void OrderItem::set_id(uint64_t id) {
    id_ = id;
}

ProductId OrderItem::product_id() const {
    return product_id_;
}

const std::string& OrderItem::product_name() const {
    return product_name_;
}

const std::string& OrderItem::product_sku() const {
    return product_sku_;
}

void OrderItem::set_product_sku(const std::string& sku) {
    product_sku_ = sku;
}

int OrderItem::quantity() const {
    return quantity_;
}

void OrderItem::set_quantity(int qty) {
    quantity_ = qty;
}

void OrderItem::increase_quantity(int delta) {
    quantity_ += delta;
}

void OrderItem::decrease_quantity(int delta) {
    quantity_ -= delta;
    if (quantity_ < 0) quantity_ = 0;
}

double OrderItem::unit_price() const {
    return unit_price_;
}

void OrderItem::set_unit_price(double price) {
    unit_price_ = price;
}

double OrderItem::subtotal() const {
    return unit_price_ * quantity_;
}

double OrderItem::discount_amount() const {
    return discount_amount_;
}

void OrderItem::set_discount_amount(double amount) {
    discount_amount_ = amount;
}

double OrderItem::actual_amount() const {
    return subtotal() - discount_amount_;
}

const std::map<std::string, std::string>& OrderItem::attributes() const {
    return attributes_;
}

void OrderItem::set_attribute(const std::string& key, const std::string& value) {
    attributes_[key] = value;
}

std::string OrderItem::get_attribute(const std::string& key, const std::string& default_value) const {
    auto it = attributes_.find(key);
    if (it != attributes_.end()) {
        return it->second;
    }
    return default_value;
}

const OrderItemSnapshot& OrderItem::snapshot() const {
    return snapshot_;
}

void OrderItem::take_snapshot() {
    snapshot_.product_id = product_id_;
    snapshot_.product_name = product_name_;
    snapshot_.product_sku = product_sku_;
    snapshot_.attributes = attributes_;
    has_snapshot_ = true;
}

WarehouseId OrderItem::warehouse_id() const {
    return warehouse_id_;
}

void OrderItem::set_warehouse_id(WarehouseId id) {
    warehouse_id_ = id;
}

bool OrderItem::is_gift() const {
    return is_gift_;
}

void OrderItem::set_is_gift(bool is_gift) {
    is_gift_ = is_gift;
}

const std::string& OrderItem::remark() const {
    return remark_;
}

void OrderItem::set_remark(const std::string& remark) {
    remark_ = remark;
}

std::string OrderItem::to_string() const {
    return product_name_ + " x " + std::to_string(quantity_);
}

bool OrderItem::operator==(const OrderItem& other) const {
    return id_ == other.id_;
}

bool OrderItem::operator!=(const OrderItem& other) const {
    return !(*this == other);
}

OrderItemCollection::OrderItemCollection() {
}

size_t OrderItemCollection::size() const {
    return items_.size();
}

bool OrderItemCollection::is_empty() const {
    return items_.empty();
}

void OrderItemCollection::clear() {
    items_.clear();
    id_index_.clear();
    product_index_.clear();
}

Result OrderItemCollection::add_item(const OrderItem& item) {
    OrderItem new_item = item;
    new_item.set_id(items_.size() + 1);
    items_.push_back(new_item);
    rebuild_indexes();
    return Result::ok();
}

Result OrderItemCollection::remove_item(uint64_t item_id) {
    auto it = id_index_.find(item_id);
    if (it == id_index_.end()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Item not found");
    }

    items_.erase(items_.begin() + it->second);
    rebuild_indexes();
    return Result::ok();
}

Result OrderItemCollection::remove_item_by_product(ProductId product_id) {
    auto it = product_index_.find(product_id);
    if (it == product_index_.end()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Product not found");
    }

    std::vector<size_t> indices = it->second;
    std::sort(indices.rbegin(), indices.rend());

    for (size_t idx : indices) {
        items_.erase(items_.begin() + idx);
    }
    rebuild_indexes();
    return Result::ok();
}

OrderItem* OrderItemCollection::get_item(uint64_t item_id) {
    auto it = id_index_.find(item_id);
    if (it == id_index_.end()) {
        return nullptr;
    }
    return &items_[it->second];
}

const OrderItem* OrderItemCollection::get_item(uint64_t item_id) const {
    auto it = id_index_.find(item_id);
    if (it == id_index_.end()) {
        return nullptr;
    }
    return &items_[it->second];
}

OrderItem* OrderItemCollection::get_item_by_product(ProductId product_id) {
    auto it = product_index_.find(product_id);
    if (it == product_index_.end() || it->second.empty()) {
        return nullptr;
    }
    return &items_[it->second[0]];
}

const OrderItem* OrderItemCollection::get_item_by_product(ProductId product_id) const {
    auto it = product_index_.find(product_id);
    if (it == product_index_.end() || it->second.empty()) {
        return nullptr;
    }
    return &items_[it->second[0]];
}

std::vector<OrderItem*> OrderItemCollection::all_items() {
    std::vector<OrderItem*> result;
    for (auto& item : items_) {
        result.push_back(&item);
    }
    return result;
}

const std::vector<OrderItem>& OrderItemCollection::items() const {
    return items_;
}

Result OrderItemCollection::update_quantity(uint64_t item_id, int new_quantity) {
    OrderItem* item = get_item(item_id);
    if (!item) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Item not found");
    }
    item->set_quantity(new_quantity);
    return Result::ok();
}

Result OrderItemCollection::update_price(uint64_t item_id, double new_price) {
    OrderItem* item = get_item(item_id);
    if (!item) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Item not found");
    }
    item->set_unit_price(new_price);
    return Result::ok();
}

int OrderItemCollection::total_quantity() const {
    int total = 0;
    for (const auto& item : items_) {
        total += item.quantity();
    }
    return total;
}

double OrderItemCollection::total_subtotal() const {
    double total = 0;
    for (const auto& item : items_) {
        total += item.subtotal();
    }
    return total;
}

double OrderItemCollection::total_discount() const {
    double total = 0;
    for (const auto& item : items_) {
        total += item.discount_amount();
    }
    return total;
}

double OrderItemCollection::total_actual() const {
    double total = 0;
    for (const auto& item : items_) {
        total += item.actual_amount();
    }
    return total;
}

bool OrderItemCollection::contains_product(ProductId product_id) const {
    return product_index_.count(product_id) > 0;
}

int OrderItemCollection::count_product(ProductId product_id) const {
    auto it = product_index_.find(product_id);
    if (it == product_index_.end()) {
        return 0;
    }
    int count = 0;
    for (size_t idx : it->second) {
        count += items_[idx].quantity();
    }
    return count;
}

std::vector<ProductId> OrderItemCollection::all_product_ids() const {
    std::vector<ProductId> result;
    for (const auto& pair : product_index_) {
        result.push_back(pair.first);
    }
    return result;
}

std::map<ProductId, int> OrderItemCollection::product_quantity_map() const {
    std::map<ProductId, int> result;
    for (const auto& pair : product_index_) {
        result[pair.first] = count_product(pair.first);
    }
    return result;
}

Result OrderItemCollection::merge_duplicate_products() {
    std::map<ProductId, OrderItem> merged;
    for (const auto& item : items_) {
        if (merged.count(item.product_id()) == 0) {
            merged[item.product_id()] = item;
        } else {
            merged[item.product_id()].increase_quantity(item.quantity());
        }
    }

    items_.clear();
    for (auto& pair : merged) {
        items_.push_back(pair.second);
    }
    rebuild_indexes();
    return Result::ok();
}

Result OrderItemCollection::sort_by_price(bool ascending) {
    if (ascending) {
        std::sort(items_.begin(), items_.end(),
            [](const OrderItem& a, const OrderItem& b) {
                return a.unit_price() < b.unit_price();
            });
    } else {
        std::sort(items_.begin(), items_.end(),
            [](const OrderItem& a, const OrderItem& b) {
                return a.unit_price() > b.unit_price();
            });
    }
    rebuild_indexes();
    return Result::ok();
}

Result OrderItemCollection::sort_by_quantity(bool ascending) {
    if (ascending) {
        std::sort(items_.begin(), items_.end(),
            [](const OrderItem& a, const OrderItem& b) {
                return a.quantity() < b.quantity();
            });
    } else {
        std::sort(items_.begin(), items_.end(),
            [](const OrderItem& a, const OrderItem& b) {
                return a.quantity() > b.quantity();
            });
    }
    rebuild_indexes();
    return Result::ok();
}

void OrderItemCollection::rebuild_indexes() {
    id_index_.clear();
    product_index_.clear();
    for (size_t i = 0; i < items_.size(); i++) {
        id_index_[items_[i].id()] = i;
        product_index_[items_[i].product_id()].push_back(i);
    }
}

} // namespace order
} // namespace oms
