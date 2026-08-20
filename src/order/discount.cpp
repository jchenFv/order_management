#include "order/discount.h"
#include "order/order.h"
#include <algorithm>

namespace oms {
namespace order {

MinimumAmountCondition::MinimumAmountCondition(double min_amount)
    : min_amount_(min_amount) {
}

bool MinimumAmountCondition::is_satisfied(const Order& order) const {
    return order.subtotal() >= min_amount_;
}

std::string MinimumAmountCondition::description() const {
    return "Minimum order amount: " + std::to_string(min_amount_);
}

std::string MinimumAmountCondition::to_string() const {
    return "MinimumAmountCondition(" + std::to_string(min_amount_) + ")";
}

double MinimumAmountCondition::min_amount() const {
    return min_amount_;
}

MinimumQuantityCondition::MinimumQuantityCondition(int min_qty)
    : min_quantity_(min_qty) {
}

bool MinimumQuantityCondition::is_satisfied(const Order& order) const {
    return order.items().size() >= min_quantity_;
}

std::string MinimumQuantityCondition::description() const {
    return "Minimum quantity: " + std::to_string(min_quantity_);
}

std::string MinimumQuantityCondition::to_string() const {
    return "MinimumQuantityCondition(" + std::to_string(min_quantity_) + ")";
}

int MinimumQuantityCondition::min_quantity() const {
    return min_quantity_;
}

ProductCondition::ProductCondition(const std::vector<ProductId>& product_ids)
    : product_ids_(product_ids) {
}

bool ProductCondition::is_satisfied(const Order& order) const {
    for (const auto& item : order.items().items()) {
        if (std::find(product_ids_.begin(), product_ids_.end(), item.product_id()) != product_ids_.end()) {
            return true;
        }
    }
    return false;
}

std::string ProductCondition::description() const {
    return "Applicable to " + std::to_string(product_ids_.size()) + " products";
}

std::string ProductCondition::to_string() const {
    return "ProductCondition(" + std::to_string(product_ids_.size()) + " products)";
}

const std::vector<ProductId>& ProductCondition::product_ids() const {
    return product_ids_;
}

CategoryCondition::CategoryCondition(const std::vector<std::string>& categories)
    : categories_(categories) {
}

bool CategoryCondition::is_satisfied(const Order& order) const {
    (void)order;
    return true;
}

std::string CategoryCondition::description() const {
    return "Applicable to categories";
}

std::string CategoryCondition::to_string() const {
    return "CategoryCondition(" + std::to_string(categories_.size()) + " categories)";
}

const std::vector<std::string>& CategoryCondition::categories() const {
    return categories_;
}

UserLevelCondition::UserLevelCondition(UserRole min_role)
    : min_role_(min_role) {
}

bool UserLevelCondition::is_satisfied(const Order& order) const {
    return order.user_role() >= min_role_;
}

std::string UserLevelCondition::description() const {
    return "Minimum user level required";
}

std::string UserLevelCondition::to_string() const {
    return "UserLevelCondition()";
}

UserRole UserLevelCondition::min_role() const {
    return min_role_;
}

TimeRangeCondition::TimeRangeCondition(std::chrono::system_clock::time_point start,
                                        std::chrono::system_clock::time_point end)
    : start_(start), end_(end) {
}

bool TimeRangeCondition::is_satisfied(const Order& order) const {
    (void)order;
    auto now = std::chrono::system_clock::now();
    return now >= start_ && now <= end_;
}

std::string TimeRangeCondition::description() const {
    return "Valid during specified time period";
}

std::string TimeRangeCondition::to_string() const {
    return "TimeRangeCondition()";
}

const std::chrono::system_clock::time_point& TimeRangeCondition::start_time() const {
    return start_;
}

const std::chrono::system_clock::time_point& TimeRangeCondition::end_time() const {
    return end_;
}

AndCondition::AndCondition(std::vector<std::shared_ptr<DiscountCondition>> conditions)
    : conditions_(std::move(conditions)) {
}

bool AndCondition::is_satisfied(const Order& order) const {
    for (const auto& cond : conditions_) {
        if (!cond->is_satisfied(order)) {
            return false;
        }
    }
    return true;
}

std::string AndCondition::description() const {
    return "All conditions must be satisfied";
}

std::string AndCondition::to_string() const {
    return "AndCondition(" + std::to_string(conditions_.size()) + ")";
}

const std::vector<std::shared_ptr<DiscountCondition>>& AndCondition::conditions() const {
    return conditions_;
}

OrCondition::OrCondition(std::vector<std::shared_ptr<DiscountCondition>> conditions)
    : conditions_(std::move(conditions)) {
}

bool OrCondition::is_satisfied(const Order& order) const {
    for (const auto& cond : conditions_) {
        if (cond->is_satisfied(order)) {
            return true;
        }
    }
    return false;
}

std::string OrCondition::description() const {
    return "Any condition must be satisfied";
}

std::string OrCondition::to_string() const {
    return "OrCondition(" + std::to_string(conditions_.size()) + ")";
}

const std::vector<std::shared_ptr<DiscountCondition>>& OrCondition::conditions() const {
    return conditions_;
}

PercentageDiscount::PercentageDiscount(double percentage, double max_amount)
    : percentage_(percentage), max_amount_(max_amount) {
}

double PercentageDiscount::calculate(const Order& order) const {
    double discount = order.subtotal() * percentage_ / 100.0;
    if (max_amount_ > 0 && discount > max_amount_) {
        return max_amount_;
    }
    return discount;
}

std::string PercentageDiscount::description() const {
    return std::to_string(percentage_) + "% off";
}

DiscountType PercentageDiscount::type() const {
    return DiscountType::PERCENTAGE;
}

double PercentageDiscount::percentage() const {
    return percentage_;
}

double PercentageDiscount::max_amount() const {
    return max_amount_;
}

FixedAmountDiscount::FixedAmountDiscount(double amount)
    : amount_(amount) {
}

double FixedAmountDiscount::calculate(const Order& order) const {
    (void)order;
    return amount_;
}

std::string FixedAmountDiscount::description() const {
    return std::to_string(amount_) + " off";
}

DiscountType FixedAmountDiscount::type() const {
    return DiscountType::FIXED_AMOUNT;
}

double FixedAmountDiscount::amount() const {
    return amount_;
}

BuyNGetMDiscount::BuyNGetMDiscount(int buy_count, int free_count, ProductId target_product)
    : buy_count_(buy_count), free_count_(free_count), target_product_(target_product) {
}

double BuyNGetMDiscount::calculate(const Order& order) const {
    int total_qty = 0;
    double unit_price = 0;

    for (const auto& item : order.items().items()) {
        if (target_product_ == 0 || item.product_id() == target_product_) {
            total_qty += item.quantity();
            if (unit_price == 0) {
                unit_price = item.unit_price();
            }
        }
    }

    int free_items = (total_qty / (buy_count_ + free_count_)) * free_count_;
    return free_items * unit_price;
}

std::string BuyNGetMDiscount::description() const {
    return "Buy " + std::to_string(buy_count_) + " get " + std::to_string(free_count_) + " free";
}

DiscountType BuyNGetMDiscount::type() const {
    return DiscountType::BUY_N_GET_M_FREE;
}

int BuyNGetMDiscount::buy_count() const {
    return buy_count_;
}

int BuyNGetMDiscount::free_count() const {
    return free_count_;
}

ProductId BuyNGetMDiscount::target_product() const {
    return target_product_;
}

FullReductionDiscount::FullReductionDiscount(double threshold, double reduction)
    : threshold_(threshold), reduction_(reduction) {
}

double FullReductionDiscount::calculate(const Order& order) const {
    if (order.subtotal() >= threshold_) {
        return reduction_;
    }
    return 0;
}

std::string FullReductionDiscount::description() const {
    return "Spend " + std::to_string(threshold_) + " get " + std::to_string(reduction_) + " off";
}

DiscountType FullReductionDiscount::type() const {
    return DiscountType::FULL_REDUCTION;
}

double FullReductionDiscount::threshold() const {
    return threshold_;
}

double FullReductionDiscount::reduction() const {
    return reduction_;
}

Discount::Discount()
    : id_(0), type_(DiscountType::PERCENTAGE), usage_limit_(0), usage_count_(0),
      is_active_(true), is_stackable_(false), priority_(0), max_discount_amount_(0.0) {
}

Discount::Discount(DiscountId id, const std::string& name, DiscountType type)
    : id_(id), name_(name), type_(type), usage_limit_(0), usage_count_(0),
      is_active_(true), is_stackable_(false), priority_(0), max_discount_amount_(0.0) {
}

DiscountId Discount::id() const {
    return id_;
}

const std::string& Discount::name() const {
    return name_;
}

const std::string& Discount::description() const {
    return description_;
}

void Discount::set_description(const std::string& desc) {
    description_ = desc;
}

const std::string& Discount::code() const {
    return code_;
}

void Discount::set_code(const std::string& code) {
    code_ = code;
}

DiscountType Discount::type() const {
    return type_;
}

void Discount::set_condition(std::shared_ptr<DiscountCondition> condition) {
    condition_ = std::move(condition);
}

bool Discount::is_applicable(const Order& order) const {
    if (!is_active_) return false;
    if (!is_valid_now()) return false;
    if (condition_) {
        return condition_->is_satisfied(order);
    }
    return true;
}

void Discount::set_application(std::shared_ptr<DiscountApplication> application) {
    application_ = std::move(application);
}

double Discount::calculate_discount(const Order& order) const {
    if (!application_) return 0;
    double discount = application_->calculate(order);
    if (max_discount_amount_ > 0 && discount > max_discount_amount_) {
        return max_discount_amount_;
    }
    return discount;
}

int Discount::usage_limit() const {
    return usage_limit_;
}

void Discount::set_usage_limit(int limit) {
    usage_limit_ = limit;
}

int Discount::usage_count() const {
    return usage_count_;
}

void Discount::increment_usage() {
    usage_count_++;
}

bool Discount::is_active() const {
    return is_active_;
}

void Discount::set_active(bool active) {
    is_active_ = active;
}

bool Discount::is_stackable() const {
    return is_stackable_;
}

void Discount::set_stackable(bool stackable) {
    is_stackable_ = stackable;
}

int Discount::priority() const {
    return priority_;
}

void Discount::set_priority(int priority) {
    priority_ = priority;
}

TimeRange Discount::valid_period() const {
    return valid_period_;
}

void Discount::set_valid_period(const TimeRange& range) {
    valid_period_ = range;
}

bool Discount::is_valid_now() const {
    auto now = std::chrono::system_clock::now();
    return now >= valid_period_.start && now <= valid_period_.end;
}

double Discount::max_discount_amount() const {
    return max_discount_amount_;
}

void Discount::set_max_discount_amount(double amount) {
    max_discount_amount_ = amount;
}

std::string Discount::to_string() const {
    return "Discount(" + name_ + ")";
}

DiscountManager& DiscountManager::instance() {
    static DiscountManager instance;
    return instance;
}

DiscountManager::DiscountManager() {
}

DiscountManager::~DiscountManager() {
    shutdown();
}

Result DiscountManager::init() {
    return Result::ok();
}

void DiscountManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    discounts_.clear();
    code_index_.clear();
}

ResultT<DiscountId> DiscountManager::create_discount(const std::string& name, DiscountType type) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    DiscountId id = discounts_.size() + 1;
    auto discount = std::make_unique<Discount>(id, name, type);
    discounts_[id] = std::move(discount);

    return ResultT<DiscountId>::ok(id);
}

Result DiscountManager::delete_discount(DiscountId id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = discounts_.find(id);
    if (it == discounts_.end()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Discount not found");
    }

    code_index_.erase(it->second->code());
    discounts_.erase(it);

    return Result::ok();
}

ResultT<Discount*> DiscountManager::get_discount(DiscountId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = discounts_.find(id);
    if (it == discounts_.end()) {
        return ResultT<Discount*>::error(ErrorCode::INVALID_PARAMETER, "Discount not found");
    }

    return ResultT<Discount*>::ok(it->second.get());
}

ResultT<Discount*> DiscountManager::get_discount_by_code(const std::string& code) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = code_index_.find(code);
    if (it == code_index_.end()) {
        return ResultT<Discount*>::error(ErrorCode::INVALID_PARAMETER, "Discount not found");
    }

    return get_discount(it->second);
}

Result DiscountManager::activate_discount(DiscountId id) {
    auto result = get_discount(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    result.value()->set_active(true);
    return Result::ok();
}

Result DiscountManager::deactivate_discount(DiscountId id) {
    auto result = get_discount(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    result.value()->set_active(false);
    return Result::ok();
}

ResultT<std::vector<Discount*>> DiscountManager::get_applicable_discounts(const Order& order) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Discount*> result;
    for (const auto& pair : discounts_) {
        if (pair.second->is_applicable(order)) {
            result.push_back(pair.second.get());
        }
    }

    return ResultT<std::vector<Discount*>>::ok(result);
}

ResultT<std::vector<Discount*>> DiscountManager::get_active_discounts() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Discount*> result;
    for (const auto& pair : discounts_) {
        if (pair.second->is_active()) {
            result.push_back(pair.second.get());
        }
    }

    return ResultT<std::vector<Discount*>>::ok(result);
}

double DiscountManager::calculate_total_discount(const Order& order,
                                                 const std::vector<DiscountId>& discount_ids) {
    double total = 0;
    for (DiscountId id : discount_ids) {
        auto result = get_discount(id);
        if (result) {
            total += result.value()->calculate_discount(order);
        }
    }
    return total;
}

ResultT<std::pair<double, std::vector<DiscountId>>>
DiscountManager::calculate_optimal_discounts(const Order& order, size_t max_count) {
    auto applicable_result = get_applicable_discounts(order);
    if (!applicable_result) {
        return ResultT<std::pair<double, std::vector<DiscountId>>>::error(
            applicable_result.error_code(), applicable_result.error_message());
    }

    auto applicable = applicable_result.value();

    std::sort(applicable.begin(), applicable.end(),
        [](Discount* a, Discount* b) {
            return a->priority() < b->priority();
        });

    std::vector<DiscountId> ids;
    double total = 0;

    for (size_t i = 0; i < std::min(max_count, applicable.size()); i++) {
        ids.push_back(applicable[i]->id());
        total += applicable[i]->calculate_discount(order);
    }

    return ResultT<std::pair<double, std::vector<DiscountId>>>::ok(
        std::make_pair(total, ids));
}

Result DiscountManager::validate_discount_combination(const std::vector<DiscountId>& discount_ids) {
    for (DiscountId id : discount_ids) {
        auto result = get_discount(id);
        if (!result) {
            return Result::error(result.error_code(), result.error_message());
        }
        if (!result.value()->is_stackable() && discount_ids.size() > 1) {
            return Result::error(ErrorCode::INVALID_PARAMETER, "Discount is not stackable");
        }
    }
    return Result::ok();
}

} // namespace order
} // namespace oms
