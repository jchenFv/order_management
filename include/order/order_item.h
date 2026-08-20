#ifndef OMS_ORDER_ORDER_ITEM_H
#define OMS_ORDER_ORDER_ITEM_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <map>

namespace oms {
namespace order {

struct OrderItemSnapshot {
    ProductId product_id;
    std::string product_name;
    std::string product_sku;
    std::string category;
    std::string brand;
    std::map<std::string, std::string> attributes;
};

class OrderItem {
public:
    OrderItem();
    OrderItem(ProductId product_id, const std::string& name, int quantity, double unit_price);

    uint64_t id() const;
    void set_id(uint64_t id);

    ProductId product_id() const;
    const std::string& product_name() const;
    const std::string& product_sku() const;
    void set_product_sku(const std::string& sku);

    int quantity() const;
    void set_quantity(int qty);
    void increase_quantity(int delta);
    void decrease_quantity(int delta);

    double unit_price() const;
    void set_unit_price(double price);

    double subtotal() const;
    double discount_amount() const;
    void set_discount_amount(double amount);

    double actual_amount() const;

    const std::map<std::string, std::string>& attributes() const;
    void set_attribute(const std::string& key, const std::string& value);
    std::string get_attribute(const std::string& key, const std::string& default_value = "") const;

    const OrderItemSnapshot& snapshot() const;
    void take_snapshot();

    WarehouseId warehouse_id() const;
    void set_warehouse_id(WarehouseId id);

    bool is_gift() const;
    void set_is_gift(bool is_gift);

    const std::string& remark() const;
    void set_remark(const std::string& remark);

    std::string to_string() const;

    bool operator==(const OrderItem& other) const;
    bool operator!=(const OrderItem& other) const;

private:
    uint64_t id_;
    ProductId product_id_;
    std::string product_name_;
    std::string product_sku_;
    int quantity_;
    double unit_price_;
    double discount_amount_;
    WarehouseId warehouse_id_;
    bool is_gift_;
    std::string remark_;
    std::map<std::string, std::string> attributes_;
    OrderItemSnapshot snapshot_;
    bool has_snapshot_;
};

class OrderItemCollection {
public:
    OrderItemCollection();

    size_t size() const;
    bool is_empty() const;
    void clear();

    Result add_item(const OrderItem& item);
    Result remove_item(uint64_t item_id);
    Result remove_item_by_product(ProductId product_id);

    OrderItem* get_item(uint64_t item_id);
    const OrderItem* get_item(uint64_t item_id) const;

    OrderItem* get_item_by_product(ProductId product_id);
    const OrderItem* get_item_by_product(ProductId product_id) const;

    std::vector<OrderItem*> all_items();
    const std::vector<OrderItem>& items() const;

    Result update_quantity(uint64_t item_id, int new_quantity);
    Result update_price(uint64_t item_id, double new_price);

    int total_quantity() const;
    double total_subtotal() const;
    double total_discount() const;
    double total_actual() const;

    bool contains_product(ProductId product_id) const;
    int count_product(ProductId product_id) const;

    std::vector<ProductId> all_product_ids() const;
    std::map<ProductId, int> product_quantity_map() const;

    Result merge_duplicate_products();
    Result sort_by_price(bool ascending = true);
    Result sort_by_quantity(bool ascending = true);

private:
    std::vector<OrderItem> items_;
    std::map<uint64_t, size_t> id_index_;
    std::map<ProductId, std::vector<size_t>> product_index_;

    void rebuild_indexes();
};

} // namespace order
} // namespace oms

#endif // OMS_ORDER_ORDER_ITEM_H
