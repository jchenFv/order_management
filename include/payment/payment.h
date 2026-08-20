#ifndef PAYMENT_PAYMENT_H
#define PAYMENT_PAYMENT_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <shared_mutex>
#include <chrono>
#include <functional>

namespace oms {
namespace payment {

enum class TransactionStatus {
    PENDING = 1,
    PROCESSING = 2,
    SUCCESS = 3,
    FAILED = 4,
    REFUNDED = 5,
    PARTIALLY_REFUNDED = 6,
    CANCELLED = 7,
    EXPIRED = 8
};

enum class GatewayType {
    ALIPAY = 1,
    WECHAT_PAY = 2,
    UNION_PAY = 3,
    CREDIT_CARD = 4,
    PAYPAL = 5,
    APPLE_PAY = 6,
    BANK_TRANSFER = 7
};

enum class RefundStatus {
    NONE = 0,
    REQUESTED = 1,
    PROCESSING = 2,
    COMPLETED = 3,
    FAILED = 4,
    REJECTED = 5
};

class Transaction {
public:
    Transaction();
    Transaction(TransactionId id, OrderId order_id, Money amount, PaymentMethod method);

    TransactionId id() const { return id_; }
    OrderId order_id() const { return order_id_; }
    UserId user_id() const { return user_id_; }
    void set_user_id(UserId user_id) { user_id_ = user_id; }

    Money amount() const { return amount_; }
    Money fee() const { return fee_; }
    void set_fee(Money fee) { fee_ = fee; }

    PaymentMethod method() const { return method_; }
    GatewayType gateway() const { return gateway_; }
    void set_gateway(GatewayType gateway) { gateway_ = gateway; }

    TransactionStatus status() const { return status_; }
    void set_status(TransactionStatus status) { status_ = status; }

    const std::string& gateway_transaction_id() const { return gateway_transaction_id_; }
    void set_gateway_transaction_id(const std::string& id) { gateway_transaction_id_ = id; }

    const std::string& failure_reason() const { return failure_reason_; }
    void set_failure_reason(const std::string& reason) { failure_reason_ = reason; }

    time_t created_at() const { return created_at_; }
    time_t paid_at() const { return paid_at_; }
    void set_paid_at(time_t t) { paid_at_ = t; }

    bool is_success() const { return status_ == TransactionStatus::SUCCESS; }
    bool is_pending() const { return status_ == TransactionStatus::PENDING; }
    bool is_failed() const { return status_ == TransactionStatus::FAILED; }

    Money refunded_amount() const { return refunded_amount_; }
    RefundStatus refund_status() const { return refund_status_; }
    void set_refund_status(RefundStatus status) { refund_status_ = status; }

    Result can_refund(Money amount) const;
    Result record_refund(Money amount);

    const std::string& currency() const { return currency_; }
    void set_currency(const std::string& currency) { currency_ = currency; }

    const std::map<std::string, std::string>& metadata() const { return metadata_; }
    void set_metadata(const std::string& key, const std::string& value) { metadata_[key] = value; }

    std::string to_string() const;

private:
    TransactionId id_;
    OrderId order_id_;
    UserId user_id_;
    Money amount_;
    Money fee_;
    Money refunded_amount_;
    PaymentMethod method_;
    GatewayType gateway_;
    TransactionStatus status_;
    RefundStatus refund_status_;
    std::string gateway_transaction_id_;
    std::string failure_reason_;
    std::string currency_;
    time_t created_at_;
    time_t paid_at_;
    std::map<std::string, std::string> metadata_;
};

class RefundRequest {
public:
    RefundRequest();
    RefundRequest(RefundId id, TransactionId transaction_id, Money amount);

    RefundId id() const { return id_; }
    TransactionId transaction_id() const { return transaction_id_; }
    OrderId order_id() const { return order_id_; }
    void set_order_id(OrderId order_id) { order_id_ = order_id; }

    Money amount() const { return amount_; }
    const std::string& reason() const { return reason_; }
    void set_reason(const std::string& reason) { reason_ = reason; }

    RefundStatus status() const { return status_; }
    void set_status(RefundStatus status) { status_ = status; }

    UserId requested_by() const { return requested_by_; }
    void set_requested_by(UserId user_id) { requested_by_ = user_id; }

    time_t requested_at() const { return requested_at_; }
    time_t processed_at() const { return processed_at_; }
    void set_processed_at(time_t t) { processed_at_ = t; }

    const std::string& gateway_refund_id() const { return gateway_refund_id_; }
    void set_gateway_refund_id(const std::string& id) { gateway_refund_id_ = id; }

    const std::string& reject_reason() const { return reject_reason_; }
    void set_reject_reason(const std::string& reason) { reject_reason_ = reason; }

    bool is_partial() const { return is_partial_; }
    void set_partial(bool partial) { is_partial_ = partial; }

private:
    RefundId id_;
    TransactionId transaction_id_;
    OrderId order_id_;
    Money amount_;
    std::string reason_;
    RefundStatus status_;
    UserId requested_by_;
    time_t requested_at_;
    time_t processed_at_;
    std::string gateway_refund_id_;
    std::string reject_reason_;
    bool is_partial_;
};

class PaymentGateway {
public:
    virtual ~PaymentGateway() = default;
    virtual GatewayType type() const = 0;
    virtual const std::string& name() const = 0;

    virtual ResultT<Transaction*> process_payment(Transaction* txn) = 0;
    virtual ResultT<RefundRequest*> process_refund(RefundRequest* refund) = 0;

    virtual Result query_status(Transaction* txn) = 0;
    virtual bool is_available() const = 0;

    virtual Money calculate_fee(Money amount) const = 0;
    virtual Money minimum_amount() const = 0;
    virtual Money maximum_amount() const = 0;

    virtual bool supports_currency(const std::string& currency) const = 0;
    virtual bool supports_method(PaymentMethod method) const = 0;
};

class AlipayGateway : public PaymentGateway {
public:
    AlipayGateway();

    GatewayType type() const override { return GatewayType::ALIPAY; }
    const std::string& name() const override { return name_; }

    ResultT<Transaction*> process_payment(Transaction* txn) override;
    ResultT<RefundRequest*> process_refund(RefundRequest* refund) override;

    Result query_status(Transaction* txn) override;
    bool is_available() const override;

    Money calculate_fee(Money amount) const override;
    Money minimum_amount() const override;
    Money maximum_amount() const override;

    bool supports_currency(const std::string& currency) const override;
    bool supports_method(PaymentMethod method) const override;

private:
    std::string name_;
    double fee_rate_;
    Money min_fee_;
    Money max_fee_;
    bool is_sandbox_;
};

class WechatPayGateway : public PaymentGateway {
public:
    WechatPayGateway();

    GatewayType type() const override { return GatewayType::WECHAT_PAY; }
    const std::string& name() const override { return name_; }

    ResultT<Transaction*> process_payment(Transaction* txn) override;
    ResultT<RefundRequest*> process_refund(RefundRequest* refund) override;

    Result query_status(Transaction* txn) override;
    bool is_available() const override;

    Money calculate_fee(Money amount) const override;
    Money minimum_amount() const override;
    Money maximum_amount() const override;

    bool supports_currency(const std::string& currency) const override;
    bool supports_method(PaymentMethod method) const override;

private:
    std::string name_;
    double fee_rate_;
};

class CreditCardGateway : public PaymentGateway {
public:
    CreditCardGateway();

    GatewayType type() const override { return GatewayType::CREDIT_CARD; }
    const std::string& name() const override { return name_; }

    ResultT<Transaction*> process_payment(Transaction* txn) override;
    ResultT<RefundRequest*> process_refund(RefundRequest* refund) override;

    Result query_status(Transaction* txn) override;
    bool is_available() const override;

    Money calculate_fee(Money amount) const override;
    Money minimum_amount() const override;
    Money maximum_amount() const override;

    bool supports_currency(const std::string& currency) const override;
    bool supports_method(PaymentMethod method) const override;

    Result validate_card(const std::string& card_number, const std::string& expiry, const std::string& cvv);

private:
    std::string name_;
    double fee_rate_;
    std::map<std::string, double> card_brand_rates_;
};

class PaymentManager {
public:
    static PaymentManager& instance();

    Result init();
    void shutdown();

    ResultT<Transaction*> create_transaction(OrderId order_id, UserId user_id,
                                              Money amount, PaymentMethod method);
    ResultT<Transaction*> get_transaction(TransactionId id);
    ResultT<std::vector<Transaction*>> get_transactions_by_order(OrderId order_id);
    ResultT<std::vector<Transaction*>> get_transactions_by_user(UserId user_id);
    ResultT<std::vector<Transaction*>> list_transactions(size_t page, size_t page_size);

    Result process_payment(TransactionId transaction_id);
    Result cancel_payment(TransactionId transaction_id);

    ResultT<RefundRequest*> create_refund(TransactionId transaction_id, Money amount,
                                           const std::string& reason, UserId requested_by);
    ResultT<RefundRequest*> get_refund(RefundId id);
    Result process_refund(RefundId refund_id);
    Result reject_refund(RefundId refund_id, const std::string& reason);

    Result register_gateway(std::unique_ptr<PaymentGateway> gateway);
    PaymentGateway* get_gateway(GatewayType type);
    PaymentGateway* get_default_gateway(PaymentMethod method);

    ResultT<Money> calculate_total_fees(const TimeRange& range);
    ResultT<Money> calculate_total_volume(const TimeRange& range);
    ResultT<size_t> get_success_count(const TimeRange& range);
    ResultT<size_t> get_failure_count(const TimeRange& range);

    Result retry_failed_transaction(TransactionId id);
    ResultT<std::vector<Transaction*>> get_pending_transactions();

    Result update_transaction_status(TransactionId id, TransactionStatus status);

private:
    PaymentManager();
    ~PaymentManager();

    std::string generate_client_secret() const;
    bool validate_webhook_signature(const std::string& payload, const std::string& signature) const;

    mutable std::shared_mutex mutex_;
    std::map<TransactionId, std::unique_ptr<Transaction>> transactions_;
    std::map<RefundId, std::unique_ptr<RefundRequest>> refunds_;
    std::map<GatewayType, std::unique_ptr<PaymentGateway>> gateways_;
    std::map<OrderId, std::vector<TransactionId>> order_transaction_index_;
    std::map<UserId, std::vector<TransactionId>> user_transaction_index_;
    std::map<PaymentMethod, GatewayType> default_gateways_;
};

} // namespace payment
} // namespace oms

#endif // PAYMENT_PAYMENT_H
