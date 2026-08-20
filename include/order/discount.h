#ifndef OMS_ORDER_DISCOUNT_H
#define OMS_ORDER_DISCOUNT_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <memory>

namespace oms {
namespace order {

class DiscountCondition {
public:
    virtual ~DiscountCondition() = default;
    virtual bool is_satisfied(const class Order& order) const = 0;
    virtual std::string description() const = 0;
    virtual std::string to_string() const = 0;
};

class MinimumAmountCondition : public DiscountCondition {
public:
    MinimumAmountCondition(double min_amount);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    double min_amount() const;

private:
    double min_amount_;
};

class MinimumQuantityCondition : public DiscountCondition {
public:
    MinimumQuantityCondition(int min_qty);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    int min_quantity() const;

private:
    int min_quantity_;
};

class ProductCondition : public DiscountCondition {
public:
    ProductCondition(const std::vector<ProductId>& product_ids);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    const std::vector<ProductId>& product_ids() const;

private:
    std::vector<ProductId> product_ids_;
};

class CategoryCondition : public DiscountCondition {
public:
    CategoryCondition(const std::vector<std::string>& categories);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    const std::vector<std::string>& categories() const;

private:
    std::vector<std::string> categories_;
};

class UserLevelCondition : public DiscountCondition {
public:
    UserLevelCondition(UserRole min_role);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    UserRole min_role() const;

private:
    UserRole min_role_;
};

class TimeRangeCondition : public DiscountCondition {
public:
    TimeRangeCondition(std::chrono::system_clock::time_point start,
                        std::chrono::system_clock::time_point end);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    const std::chrono::system_clock::time_point& start_time() const;
    const std::chrono::system_clock::time_point& end_time() const;

private:
    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point end_;
};

class AndCondition : public DiscountCondition {
public:
    AndCondition(std::vector<std::shared_ptr<DiscountCondition>> conditions);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    const std::vector<std::shared_ptr<DiscountCondition>>& conditions() const;

private:
    std::vector<std::shared_ptr<DiscountCondition>> conditions_;
};

class OrCondition : public DiscountCondition {
public:
    OrCondition(std::vector<std::shared_ptr<DiscountCondition>> conditions);
    bool is_satisfied(const Order& order) const override;
    std::string description() const override;
    std::string to_string() const override;

    const std::vector<std::shared_ptr<DiscountCondition>>& conditions() const;

private:
    std::vector<std::shared_ptr<DiscountCondition>> conditions_;
};

class DiscountApplication {
public:
    virtual ~DiscountApplication() = default;
    virtual double calculate(const Order& order) const = 0;
    virtual std::string description() const = 0;
    virtual DiscountType type() const = 0;
};

class PercentageDiscount : public DiscountApplication {
public:
    PercentageDiscount(double percentage, double max_amount = 0.0);
    double calculate(const Order& order) const override;
    std::string description() const override;
    DiscountType type() const override;

    double percentage() const;
    double max_amount() const;

private:
    double percentage_;
    double max_amount_;
};

class FixedAmountDiscount : public DiscountApplication {
public:
    FixedAmountDiscount(double amount);
    double calculate(const Order& order) const override;
    std::string description() const override;
    DiscountType type() const override;

    double amount() const;

private:
    double amount_;
};

class BuyNGetMDiscount : public DiscountApplication {
public:
    BuyNGetMDiscount(int buy_count, int free_count, ProductId target_product = 0);
    double calculate(const Order& order) const override;
    std::string description() const override;
    DiscountType type() const override;

    int buy_count() const;
    int free_count() const;
    ProductId target_product() const;

private:
    int buy_count_;
    int free_count_;
    ProductId target_product_;
};

class FullReductionDiscount : public DiscountApplication {
public:
    FullReductionDiscount(double threshold, double reduction);
    double calculate(const Order& order) const override;
    std::string description() const override;
    DiscountType type() const override;

    double threshold() const;
    double reduction() const;

private:
    double threshold_;
    double reduction_;
};

class Discount {
public:
    Discount();
    Discount(DiscountId id, const std::string& name, DiscountType type);

    DiscountId id() const;
    const std::string& name() const;
    const std::string& description() const;
    void set_description(const std::string& desc);

    const std::string& code() const;
    void set_code(const std::string& code);

    DiscountType type() const;

    void set_condition(std::shared_ptr<DiscountCondition> condition);
    bool is_applicable(const Order& order) const;

    void set_application(std::shared_ptr<DiscountApplication> application);
    double calculate_discount(const Order& order) const;

    int usage_limit() const;
    void set_usage_limit(int limit);
    int usage_count() const;
    void increment_usage();

    bool is_active() const;
    void set_active(bool active);

    bool is_stackable() const;
    void set_stackable(bool stackable);

    int priority() const;
    void set_priority(int priority);

    TimeRange valid_period() const;
    void set_valid_period(const TimeRange& range);
    bool is_valid_now() const;

    double max_discount_amount() const;
    void set_max_discount_amount(double amount);

    std::string to_string() const;

private:
    DiscountId id_;
    std::string name_;
    std::string description_;
    std::string code_;
    DiscountType type_;
    std::shared_ptr<DiscountCondition> condition_;
    std::shared_ptr<DiscountApplication> application_;
    int usage_limit_;
    int usage_count_;
    bool is_active_;
    bool is_stackable_;
    int priority_;
    TimeRange valid_period_;
    double max_discount_amount_;
};

class DiscountManager {
public:
    static DiscountManager& instance();

    Result init();
    void shutdown();

    ResultT<DiscountId> create_discount(const std::string& name, DiscountType type);
    Result delete_discount(DiscountId id);
    ResultT<Discount*> get_discount(DiscountId id);
    ResultT<Discount*> get_discount_by_code(const std::string& code);

    Result activate_discount(DiscountId id);
    Result deactivate_discount(DiscountId id);

    ResultT<std::vector<Discount*>> get_applicable_discounts(const Order& order);
    ResultT<std::vector<Discount*>> get_active_discounts();

    double calculate_total_discount(const Order& order,
                                     const std::vector<DiscountId>& discount_ids);

    ResultT<std::pair<double, std::vector<DiscountId>>>
    calculate_optimal_discounts(const Order& order, size_t max_count = 3);

    Result validate_discount_combination(const std::vector<DiscountId>& discount_ids);

    Result apply_points_discount(Order& order, int points);
    Result apply_coupon_code(Order& order, const std::string& code);

    ResultT<double> calculate_stackable_discounts(Order& order, const std::vector<DiscountId>& ids);

private:
    DiscountManager();
    ~DiscountManager();

    double apply_discount_recursive(Order& order, size_t index, const std::vector<DiscountId>& ids);

    std::map<DiscountId, std::unique_ptr<Discount>> discounts_;
    std::map<std::string, DiscountId> code_index_;
    std::map<std::string, std::pair<double, time_t>> coupon_cache_;
    mutable std::shared_mutex mutex_;
};

} // namespace order
} // namespace oms

#endif // OMS_ORDER_DISCOUNT_H
