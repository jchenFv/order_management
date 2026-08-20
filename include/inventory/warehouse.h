#ifndef OMS_INVENTORY_WAREHOUSE_H
#define OMS_INVENTORY_WAREHOUSE_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace oms {
namespace inventory {

struct WarehouseAddress {
    std::string street;
    std::string city;
    std::string state;
    std::string postal_code;
    std::string country;
    double latitude;
    double longitude;
};

struct WarehouseContact {
    std::string manager_name;
    std::string phone;
    std::string email;
    std::string working_hours;
};

struct WarehouseCapabilities {
    bool can_ship_domestic;
    bool can_ship_international;
    bool can_receive_returns;
    bool support_cold_chain;
    bool support_hazardous;
    int max_pallets;
    int max_weight_kg;
};

class Warehouse {
public:
    Warehouse();
    Warehouse(WarehouseId id, const std::string& name, const std::string& code);

    WarehouseId id() const;
    const std::string& name() const;
    void set_name(const std::string& name);

    const std::string& code() const;
    void set_code(const std::string& code);

    const WarehouseAddress& address() const;
    WarehouseAddress& address();
    void set_address(const WarehouseAddress& addr);

    const WarehouseContact& contact() const;
    WarehouseContact& contact();
    void set_contact(const WarehouseContact& contact);

    const WarehouseCapabilities& capabilities() const;
    WarehouseCapabilities& capabilities();
    void set_capabilities(const WarehouseCapabilities& caps);

    int priority() const;
    void set_priority(int priority);

    bool is_active() const;
    void set_active(bool active);

    bool is_default() const;
    void set_default(bool is_default);

    double shipping_cost_multiplier() const;
    void set_shipping_cost_multiplier(double multiplier);

    int get_product_stock(ProductId product_id) const;
    void set_product_stock(ProductId product_id, int quantity);
    int adjust_product_stock(ProductId product_id, int delta);

    bool has_product(ProductId product_id) const;
    bool has_available_stock(ProductId product_id, int quantity) const;

    const std::map<ProductId, int>& all_stock() const;

    int get_reserved_stock(ProductId product_id) const;
    void set_reserved_stock(ProductId product_id, int quantity);

    Result reserve_stock(ProductId product_id, int quantity);
    Result release_stock(ProductId product_id, int quantity);
    Result confirm_deduction(ProductId product_id, int quantity);

    double distance_to(double lat, double lng) const;

    time_t created_at() const;
    time_t updated_at() const;
    void update_timestamp();

    std::string to_string() const;
    std::string to_json() const;

private:
    WarehouseId id_;
    std::string name_;
    std::string code_;
    WarehouseAddress address_;
    WarehouseContact contact_;
    WarehouseCapabilities capabilities_;
    int priority_;
    bool is_active_;
    bool is_default_;
    double shipping_cost_multiplier_;
    std::map<ProductId, int> stock_;
    std::map<ProductId, int> reserved_stock_;
    time_t created_at_;
    time_t updated_at_;
};

class WarehouseManager {
public:
    static WarehouseManager& instance();

    Result init();
    void shutdown();

    ResultT<WarehouseId> create_warehouse(const std::string& name, const std::string& code);
    Result delete_warehouse(WarehouseId id);

    ResultT<Warehouse*> get_warehouse(WarehouseId id);
    ResultT<Warehouse*> get_warehouse_by_code(const std::string& code);
    ResultT<Warehouse*> get_default_warehouse();

    ResultT<std::vector<Warehouse*>> list_warehouses();
    ResultT<std::vector<Warehouse*>> list_active_warehouses();

    Result activate_warehouse(WarehouseId id);
    Result deactivate_warehouse(WarehouseId id);

    Result set_default_warehouse(WarehouseId id);

    ResultT<std::vector<Warehouse*>> find_warehouses_with_product(ProductId product_id,
                                                                     int min_quantity = 1);

    ResultT<Warehouse*> find_nearest_warehouse(double lat, double lng, ProductId product_id = 0);

    ResultT<std::map<WarehouseId, int>> get_product_distribution(ProductId product_id);
    ResultT<int> get_total_product_stock(ProductId product_id);

    Result transfer_stock(ProductId product_id,
                           WarehouseId from_warehouse,
                           WarehouseId to_warehouse,
                           int quantity,
                           const std::string& reason = "");

    Result receive_stock(ProductId product_id, WarehouseId warehouse_id,
                          int quantity, const std::string& reason = "");

    Result adjust_inventory(ProductId product_id, WarehouseId warehouse_id,
                             int quantity, const std::string& reason = "");

    ResultT<std::pair<WarehouseId, int>> allocate_order(
        OrderId order_id,
        const std::vector<std::pair<ProductId, int>>& items,
        const std::string& shipping_address = "");

    Result release_allocation(OrderId order_id);

private:
    WarehouseManager();
    ~WarehouseManager();

    std::map<WarehouseId, std::unique_ptr<Warehouse>> warehouses_;
    std::map<std::string, WarehouseId> code_index_;
    std::map<OrderId, std::map<WarehouseId, std::map<ProductId, int>>> allocations_;
    WarehouseId default_warehouse_id_;
    mutable std::shared_mutex mutex_;
};

} // namespace inventory
} // namespace oms

#endif // OMS_INVENTORY_WAREHOUSE_H
