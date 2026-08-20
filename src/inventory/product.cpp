#include "inventory/product.h"
#include "inventory/warehouse.h"
#include <algorithm>
#include <cstring>

namespace oms {
namespace inventory {

Product::Product()
    : id_(0), low_stock_threshold_(5), is_active_(true), is_visible_(true),
      is_featured_(false), weight_grams_(0.0), volume_cubic_cm_(0.0),
      created_at_(time(nullptr)), updated_at_(time(nullptr)) {
    memset(&price_info_, 0, sizeof(price_info_));
    memset(&stock_info_, 0, sizeof(stock_info_));
}

Product::Product(ProductId id, const std::string& name, const std::string& sku)
    : id_(id), name_(name), sku_(sku), low_stock_threshold_(5),
      is_active_(true), is_visible_(true), is_featured_(false),
      weight_grams_(0.0), volume_cubic_cm_(0.0),
      created_at_(time(nullptr)), updated_at_(time(nullptr)) {
    memset(&price_info_, 0, sizeof(price_info_));
    memset(&stock_info_, 0, sizeof(stock_info_));
}

ProductId Product::id() const {
    return id_;
}

const std::string& Product::name() const {
    return name_;
}

void Product::set_name(const std::string& name) {
    name_ = name;
    update_timestamp();
}

const std::string& Product::sku() const {
    return sku_;
}

void Product::set_sku(const std::string& sku) {
    sku_ = sku;
    update_timestamp();
}

const std::string& Product::description() const {
    return description_;
}

void Product::set_description(const std::string& desc) {
    description_ = desc;
    update_timestamp();
}

const std::string& Product::short_description() const {
    return short_description_;
}

void Product::set_short_description(const std::string& desc) {
    short_description_ = desc;
    update_timestamp();
}

const std::string& Product::category() const {
    return category_;
}

void Product::set_category(const std::string& cat) {
    category_ = cat;
    update_timestamp();
}

const std::string& Product::brand() const {
    return brand_;
}

void Product::set_brand(const std::string& b) {
    brand_ = b;
    update_timestamp();
}

const ProductPrice& Product::price_info() const {
    return price_info_;
}

ProductPrice& Product::price_info() {
    return price_info_;
}

void Product::set_price_info(const ProductPrice& price) {
    price_info_ = price;
    update_timestamp();
}

double Product::price() const {
    return price_info_.price;
}

void Product::set_price(double p) {
    price_info_.price = p;
    update_timestamp();
}

double Product::cost() const {
    return price_info_.cost;
}

void Product::set_cost(double c) {
    price_info_.cost = c;
    update_timestamp();
}

const ProductStock& Product::stock_info() const {
    return stock_info_;
}

ProductStock& Product::stock_info() {
    return stock_info_;
}

void Product::set_stock_info(const ProductStock& stock) {
    stock_info_ = stock;
    update_timestamp();
}

int Product::total_stock() const {
    return stock_info_.total_available;
}

int Product::available_stock() const {
    return stock_info_.total_available - stock_info_.total_reserved;
}

int Product::reserved_stock() const {
    return stock_info_.total_reserved;
}

bool Product::is_in_stock() const {
    return available_stock() > 0;
}

bool Product::is_low_stock() const {
    return available_stock() <= low_stock_threshold_;
}

bool Product::is_out_of_stock() const {
    return available_stock() <= 0;
}

int Product::low_stock_threshold() const {
    return low_stock_threshold_;
}

void Product::set_low_stock_threshold(int threshold) {
    low_stock_threshold_ = threshold;
}

bool Product::is_active() const {
    return is_active_;
}

void Product::set_active(bool active) {
    is_active_ = active;
    update_timestamp();
}

bool Product::is_visible() const {
    return is_visible_;
}

void Product::set_visible(bool visible) {
    is_visible_ = visible;
    update_timestamp();
}

bool Product::is_featured() const {
    return is_featured_;
}

void Product::set_featured(bool featured) {
    is_featured_ = featured;
    update_timestamp();
}

const std::vector<ProductAttribute>& Product::attributes() const {
    return attributes_;
}

void Product::add_attribute(const std::string& name, const std::string& value) {
    for (auto& attr : attributes_) {
        if (attr.name == name) {
            attr.value = value;
            return;
        }
    }
    attributes_.push_back({name, value});
}

void Product::remove_attribute(const std::string& name) {
    auto it = std::remove_if(attributes_.begin(), attributes_.end(),
        [&name](const ProductAttribute& attr) { return attr.name == name; });
    attributes_.erase(it, attributes_.end());
}

std::string Product::get_attribute(const std::string& name, const std::string& default_value) const {
    for (const auto& attr : attributes_) {
        if (attr.name == name) {
            return attr.value;
        }
    }
    return default_value;
}

const std::vector<ProductImage>& Product::images() const {
    return images_;
}

void Product::add_image(const std::string& url, const std::string& alt_text, bool is_primary) {
    if (is_primary) {
        for (auto& img : images_) {
            img.is_primary = false;
        }
    }
    images_.push_back({url, alt_text, is_primary, static_cast<int>(images_.size())});
}

void Product::set_primary_image(const std::string& url) {
    for (auto& img : images_) {
        img.is_primary = (img.url == url);
    }
}

void Product::clear_images() {
    images_.clear();
}

double Product::weight() const {
    return weight_grams_;
}

void Product::set_weight(double grams) {
    weight_grams_ = grams;
    update_timestamp();
}

double Product::volume() const {
    return volume_cubic_cm_;
}

void Product::set_volume(double cubic_cm) {
    volume_cubic_cm_ = cubic_cm;
    update_timestamp();
}

const std::string& Product::barcode() const {
    return barcode_;
}

void Product::set_barcode(const std::string& barcode) {
    barcode_ = barcode;
    update_timestamp();
}

const std::map<std::string, std::string>& Product::metadata() const {
    return metadata_;
}

void Product::set_metadata(const std::string& key, const std::string& value) {
    metadata_[key] = value;
    update_timestamp();
}

std::string Product::get_metadata(const std::string& key, const std::string& default_value) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second;
    }
    return default_value;
}

time_t Product::created_at() const {
    return created_at_;
}

time_t Product::updated_at() const {
    return updated_at_;
}

void Product::update_timestamp() {
    updated_at_ = time(nullptr);
}

std::string Product::to_string() const {
    return name_ + " (" + sku_ + ")";
}

std::string Product::to_json() const {
    return "{\"id\":" + std::to_string(id_) + ",\"name\":\"" + name_ + "\",\"sku\":\"" + sku_ + "\"}";
}

bool Product::operator==(const Product& other) const {
    return id_ == other.id_;
}

bool Product::operator!=(const Product& other) const {
    return !(*this == other);
}

ProductCatalog& ProductCatalog::instance() {
    static ProductCatalog instance;
    return instance;
}

ProductCatalog::ProductCatalog()
    : next_product_id_(1) {
}

ProductCatalog::~ProductCatalog() {
}

Result ProductCatalog::init() {
    return Result::ok();
}

void ProductCatalog::shutdown() {
    products_.clear();
    sku_index_.clear();
    category_index_.clear();
    brand_index_.clear();
}

ResultT<ProductId> ProductCatalog::add_product(const std::string& name, const std::string& sku, double price, int initial_stock) {
    if (sku_index_.count(sku) > 0) {
        return ResultT<ProductId>::error(ErrorCode::INVALID_PARAMETER, "SKU already exists");
    }

    ProductId id = next_product_id_++;
    auto product = std::make_unique<Product>(id, name, sku);
    product->set_price(price);
    product->stock_info().total_available = initial_stock;

    products_[id] = std::move(product);
    sku_index_[sku] = id;

    return ResultT<ProductId>::ok(id);
}

Result ProductCatalog::remove_product(ProductId id) {
    auto it = products_.find(id);
    if (it == products_.end()) {
        return Result::error(ErrorCode::PRODUCT_NOT_FOUND, "Product not found");
    }

    sku_index_.erase(it->second->sku());
    products_.erase(it);
    return Result::ok();
}

ResultT<Product*> ProductCatalog::get_product(ProductId id) {
    auto it = products_.find(id);
    if (it == products_.end()) {
        return ResultT<Product*>::error(ErrorCode::PRODUCT_NOT_FOUND, "Product not found");
    }
    return ResultT<Product*>::ok(it->second.get());
}

ResultT<Product*> ProductCatalog::get_product_by_sku(const std::string& sku) {
    auto it = sku_index_.find(sku);
    if (it == sku_index_.end()) {
        return ResultT<Product*>::error(ErrorCode::PRODUCT_NOT_FOUND, "Product not found");
    }
    return get_product(it->second);
}

ResultT<std::vector<Product*>> ProductCatalog::get_product_by_name(const std::string& name, bool partial_match) {
    std::vector<Product*> result;
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    for (const auto& pair : products_) {
        std::string product_name = pair.second->name();
        std::transform(product_name.begin(), product_name.end(), product_name.begin(), ::tolower);

        if (partial_match) {
            if (product_name.find(lower_name) != std::string::npos) {
                result.push_back(pair.second.get());
            }
        } else {
            if (product_name == lower_name) {
                result.push_back(pair.second.get());
            }
        }
    }

    return ResultT<std::vector<Product*>>::ok(result);
}

Result ProductCatalog::update_product(ProductId id, const Product& updated) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    Product* product = result.value();
    product->set_name(updated.name());
    product->set_description(updated.description());
    product->set_category(updated.category());
    product->set_brand(updated.brand());
    product->set_price(updated.price());
    product->set_cost(updated.cost());
    product->update_timestamp();

    return Result::ok();
}

Result ProductCatalog::update_price(ProductId id, double new_price) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->set_price(new_price);
    return Result::ok();
}

Result ProductCatalog::update_stock(ProductId id, WarehouseId warehouse_id, int new_stock) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    auto& stock_info = result.value()->stock_info();
    if (warehouse_id == 0) {
        stock_info.total_available = new_stock;
    } else {
        stock_info.warehouse_stock[warehouse_id] = new_stock;
    }

    return Result::ok();
}

ResultT<std::vector<Product*>> ProductCatalog::list_products(size_t page, size_t page_size) {
    std::vector<Product*> result;
    size_t start = page * page_size;
    size_t end = start + page_size;
    size_t index = 0;

    for (const auto& pair : products_) {
        if (index >= start && index < end) {
            result.push_back(pair.second.get());
        }
        index++;
        if (index >= end) {
            break;
        }
    }

    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<std::vector<Product*>> ProductCatalog::list_products_by_category(const std::string& category, size_t page, size_t page_size) {
    std::vector<Product*> result;
    size_t start = page * page_size;
    size_t end = start + page_size;
    size_t index = 0;

    for (const auto& pair : products_) {
        if (pair.second->category() == category) {
            if (index >= start && index < end) {
                result.push_back(pair.second.get());
            }
            index++;
            if (index >= end) {
                break;
            }
        }
    }

    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<std::vector<Product*>> ProductCatalog::list_products_by_brand(const std::string& brand, size_t page, size_t page_size) {
    std::vector<Product*> result;
    size_t start = page * page_size;
    size_t end = start + page_size;
    size_t index = 0;

    for (const auto& pair : products_) {
        if (pair.second->brand() == brand) {
            if (index >= start && index < end) {
                result.push_back(pair.second.get());
            }
            index++;
            if (index >= end) {
                break;
            }
        }
    }

    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<std::vector<Product*>> ProductCatalog::search_products(const std::string& keyword, size_t page, size_t page_size) {
    std::vector<Product*> result;
    std::string lower_keyword = keyword;
    std::transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(), ::tolower);

    size_t start = page * page_size;
    size_t end = start + page_size;
    size_t index = 0;

    for (const auto& pair : products_) {
        const auto& product = pair.second;
        std::string name = product->name();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name.find(lower_keyword) != std::string::npos ||
            product->sku().find(keyword) != std::string::npos) {
            if (index >= start && index < end) {
                result.push_back(product.get());
            }
            index++;
            if (index >= end) {
                break;
            }
        }
    }

    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<std::vector<Product*>> ProductCatalog::get_low_stock_products() {
    std::vector<Product*> result;
    for (const auto& pair : products_) {
        if (pair.second->is_low_stock()) {
            result.push_back(pair.second.get());
        }
    }
    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<std::vector<Product*>> ProductCatalog::get_out_of_stock_products() {
    std::vector<Product*> result;
    for (const auto& pair : products_) {
        if (pair.second->is_out_of_stock()) {
            result.push_back(pair.second.get());
        }
    }
    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<std::vector<Product*>> ProductCatalog::get_featured_products() {
    std::vector<Product*> result;
    for (const auto& pair : products_) {
        if (pair.second->is_featured()) {
            result.push_back(pair.second.get());
        }
    }
    return ResultT<std::vector<Product*>>::ok(result);
}

ResultT<size_t> ProductCatalog::get_total_product_count() const {
    return ResultT<size_t>::ok(products_.size());
}

ResultT<size_t> ProductCatalog::get_active_product_count() const {
    size_t count = 0;
    for (const auto& pair : products_) {
        if (pair.second->is_active()) {
            count++;
        }
    }
    return ResultT<size_t>::ok(count);
}

Result ProductCatalog::restock_product(ProductId id, WarehouseId warehouse_id, int quantity, const std::string& reason) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    auto& stock_info = result.value()->stock_info();
    if (warehouse_id == 0) {
        stock_info.total_available += quantity;
    } else {
        stock_info.warehouse_stock[warehouse_id] += quantity;
    }

    return Result::ok();
}

Result ProductCatalog::adjust_stock(ProductId id, WarehouseId warehouse_id, int adjustment, const std::string& reason) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    auto& stock_info = result.value()->stock_info();
    if (warehouse_id == 0) {
        stock_info.total_available += adjustment;
    } else {
        stock_info.warehouse_stock[warehouse_id] += adjustment;
    }

    return Result::ok();
}

Result ProductCatalog::activate_product(ProductId id) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->set_active(true);
    return Result::ok();
}

Result ProductCatalog::deactivate_product(ProductId id) {
    auto result = get_product(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->set_active(false);
    return Result::ok();
}

Result ProductCatalog::import_from_csv(const std::string& file_path) {
    (void)file_path;
    return Result::error(ErrorCode::NOT_IMPLEMENTED, "CSV import not implemented");
}

Result ProductCatalog::export_to_csv(const std::string& file_path) const {
    (void)file_path;
    return Result::error(ErrorCode::NOT_IMPLEMENTED, "CSV export not implemented");
}

ResultT<std::map<WarehouseId, int>> ProductCatalog::get_product_stock_distribution(ProductId product_id) {
    auto result = get_product(product_id);
    if (!result) {
        return ResultT<std::map<WarehouseId, int>>::error(result.error_code(), result.error_message());
    }

    return ResultT<std::map<WarehouseId, int>>::ok(result.value()->stock_info().warehouse_stock);
}

ResultT<int> ProductCatalog::get_product_total_stock(ProductId product_id) {
    auto result = get_product(product_id);
    if (!result) {
        return ResultT<int>::error(result.error_code(), result.error_message());
    }

    int total = result.value()->stock_info().total_available;
    for (const auto& pair : result.value()->stock_info().warehouse_stock) {
        total += pair.second;
    }

    return ResultT<int>::ok(total);
}

Result ProductCatalog::batch_update_prices(const std::vector<ProductId>& ids, double percentage) {
    for (size_t i = 0; i < ids.size(); i++) {
        auto result = get_product(ids[i]);
        if (result) {
            double new_price = result.value()->price() * (1 + percentage / 100);
            result.value()->set_price(new_price);
        }
    }
    return Result::ok();
}

Result ProductCatalog::batch_update_stock(const std::map<ProductId, int>& stock_changes) {
    auto& whm = WarehouseManager::instance();
    auto wh_result = whm.get_default_warehouse();
    WarehouseId wh_id = 0;
    if (wh_result) {
        wh_id = wh_result.value()->id();
    }

    for (const auto& pair : stock_changes) {
        ProductId product_id = pair.first;
        int delta = pair.second;

        auto result = get_product(product_id);
        if (!result) continue;

        auto product = result.value();
        auto& stock_info = product->stock_info();

        stock_info.total_available += delta;
        if (wh_id > 0) {
            stock_info.warehouse_stock[wh_id] += delta;
        }

        product->update_timestamp();
    }

    return Result::ok();
}

void ProductCatalog::update_indexes(Product* product, const std::string& old_category, const std::string& old_brand) {
    if (old_category != product->category()) {
        auto& vec = category_index_[old_category];
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == product->id()) {
                vec[i] = vec.back();
                vec.pop_back();
                break;
            }
        }
        category_index_[product->category()].push_back(product->id());
    }

    if (old_brand != product->brand()) {
        auto& vec = brand_index_[old_brand];
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == product->id()) {
                vec[i] = vec.back();
                vec.pop_back();
                break;
            }
        }
        brand_index_[product->brand()].push_back(product->id());
    }
}

} // namespace inventory
} // namespace oms
