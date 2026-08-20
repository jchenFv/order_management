#ifndef OMS_INVENTORY_PRODUCT_H
#define OMS_INVENTORY_PRODUCT_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <map>

namespace oms {
namespace inventory {

struct ProductAttribute {
    std::string name;
    std::string value;
};

struct ProductImage {
    std::string url;
    std::string alt_text;
    bool is_primary;
    int sort_order;
};

struct ProductPrice {
    double price;
    double cost;
    double msrp;
    double wholesale_price;
    std::string currency;
    bool has_tax;
    double tax_rate;
};

struct ProductStock {
    int total_available;
    int total_reserved;
    int total_in_transit;
    int total_damaged;
    std::map<WarehouseId, int> warehouse_stock;
    std::map<WarehouseId, int> warehouse_reserved;
};

class Product {
public:
    Product();
    Product(ProductId id, const std::string& name, const std::string& sku);

    ProductId id() const;
    const std::string& name() const;
    void set_name(const std::string& name);

    const std::string& sku() const;
    void set_sku(const std::string& sku);

    const std::string& description() const;
    void set_description(const std::string& desc);

    const std::string& short_description() const;
    void set_short_description(const std::string& desc);

    const std::string& category() const;
    void set_category(const std::string& category);

    const std::string& brand() const;
    void set_brand(const std::string& brand);

    const ProductPrice& price_info() const;
    ProductPrice& price_info();
    void set_price_info(const ProductPrice& price);

    double price() const;
    void set_price(double price);
    double cost() const;
    void set_cost(double cost);

    const ProductStock& stock_info() const;
    ProductStock& stock_info();
    void set_stock_info(const ProductStock& stock);

    int total_stock() const;
    int available_stock() const;
    int reserved_stock() const;

    bool is_in_stock() const;
    bool is_low_stock() const;
    bool is_out_of_stock() const;

    int low_stock_threshold() const;
    void set_low_stock_threshold(int threshold);

    bool is_active() const;
    void set_active(bool active);

    bool is_visible() const;
    void set_visible(bool visible);

    bool is_featured() const;
    void set_featured(bool featured);

    const std::vector<ProductAttribute>& attributes() const;
    void add_attribute(const std::string& name, const std::string& value);
    void remove_attribute(const std::string& name);
    std::string get_attribute(const std::string& name, const std::string& default_value = "") const;

    const std::vector<ProductImage>& images() const;
    void add_image(const std::string& url, const std::string& alt_text = "", bool is_primary = false);
    void set_primary_image(const std::string& url);
    void clear_images();

    double weight() const;
    void set_weight(double grams);
    double volume() const;
    void set_volume(double cubic_cm);

    const std::string& barcode() const;
    void set_barcode(const std::string& barcode);

    const std::map<std::string, std::string>& metadata() const;
    void set_metadata(const std::string& key, const std::string& value);
    std::string get_metadata(const std::string& key, const std::string& default_value = "") const;

    time_t created_at() const;
    time_t updated_at() const;
    void update_timestamp();

    std::string to_string() const;
    std::string to_json() const;

    bool operator==(const Product& other) const;
    bool operator!=(const Product& other) const;

private:
    ProductId id_;
    std::string name_;
    std::string sku_;
    std::string description_;
    std::string short_description_;
    std::string category_;
    std::string brand_;
    ProductPrice price_info_;
    ProductStock stock_info_;
    int low_stock_threshold_;
    bool is_active_;
    bool is_visible_;
    bool is_featured_;
    std::vector<ProductAttribute> attributes_;
    std::vector<ProductImage> images_;
    double weight_grams_;
    double volume_cubic_cm_;
    std::string barcode_;
    std::map<std::string, std::string> metadata_;
    time_t created_at_;
    time_t updated_at_;
};

class ProductCatalog {
public:
    static ProductCatalog& instance();

    Result init();
    void shutdown();

    ResultT<ProductId> add_product(const std::string& name, const std::string& sku,
                                    double price, int initial_stock);
    Result remove_product(ProductId id);

    ResultT<Product*> get_product(ProductId id);
    ResultT<Product*> get_product_by_sku(const std::string& sku);
    ResultT<std::vector<Product*>> get_product_by_name(const std::string& name,
                                                         bool partial_match = true);

    Result update_product(ProductId id, const Product& updated);
    Result update_price(ProductId id, double new_price);
    Result update_stock(ProductId id, WarehouseId warehouse_id, int new_stock);

    ResultT<std::vector<Product*>> list_products(size_t page = 0, size_t page_size = 50);
    ResultT<std::vector<Product*>> list_products_by_category(const std::string& category,
                                                               size_t page = 0, size_t page_size = 50);
    ResultT<std::vector<Product*>> list_products_by_brand(const std::string& brand,
                                                            size_t page = 0, size_t page_size = 50);

    ResultT<std::vector<Product*>> search_products(const std::string& keyword,
                                                     size_t page = 0, size_t page_size = 50);

    ResultT<std::vector<Product*>> get_low_stock_products();
    ResultT<std::vector<Product*>> get_out_of_stock_products();
    ResultT<std::vector<Product*>> get_featured_products();

    ResultT<size_t> get_total_product_count() const;
    ResultT<size_t> get_active_product_count() const;

    Result restock_product(ProductId id, WarehouseId warehouse_id, int quantity,
                            const std::string& reason = "");
    Result adjust_stock(ProductId id, WarehouseId warehouse_id, int adjustment,
                         const std::string& reason = "");

    Result activate_product(ProductId id);
    Result deactivate_product(ProductId id);

    Result import_from_csv(const std::string& file_path);
    Result export_to_csv(const std::string& file_path) const;

private:
    ProductCatalog();
    ~ProductCatalog();

    std::map<ProductId, std::unique_ptr<Product>> products_;
    std::map<std::string, ProductId> sku_index_;
    std::map<std::string, std::vector<ProductId>> category_index_;
    std::map<std::string, std::vector<ProductId>> brand_index_;
    mutable std::shared_mutex mutex_;
    ProductId next_product_id_;
};

} // namespace inventory
} // namespace oms

#endif // OMS_INVENTORY_PRODUCT_H
