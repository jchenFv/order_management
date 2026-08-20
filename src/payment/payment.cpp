#include "payment/payment.h"
#include "common/config.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>

namespace oms {
namespace payment {

Transaction::Transaction()
    : id_(0), order_id_(0), user_id_(0), amount_(0), fee_(0), refunded_amount_(0),
      method_(PaymentMethod::UNKNOWN), gateway_(GatewayType::ALIPAY),
      status_(TransactionStatus::PENDING), refund_status_(RefundStatus::NONE),
      currency_("CNY"), created_at_(time(nullptr)), paid_at_(0) {
}

Transaction::Transaction(TransactionId id, OrderId order_id, Money amount, PaymentMethod method)
    : id_(id), order_id_(order_id), user_id_(0), amount_(amount), fee_(0), refunded_amount_(0),
      method_(method), gateway_(GatewayType::ALIPAY), status_(TransactionStatus::PENDING),
      refund_status_(RefundStatus::NONE), currency_("CNY"),
      created_at_(time(nullptr)), paid_at_(0) {
}

Result Transaction::can_refund(Money amount) const {
    if (status_ != TransactionStatus::SUCCESS) {
        return Result::error(ErrorCode::PAYMENT_INVALID_STATUS, "Only successful transactions can be refunded");
    }
    Money remaining = amount_ - refunded_amount_;
    if (amount > remaining) {
        return Result::error(ErrorCode::PAYMENT_REFUND_EXCEEDS_AMOUNT, "Refund amount exceeds available amount");
    }
    if (amount <= 0) {
        return Result::error(ErrorCode::PAYMENT_INVALID_AMOUNT, "Refund amount must be positive");
    }
    return Result::ok();
}

Result Transaction::record_refund(Money amount) {
    Result check = can_refund(amount);
    if (!check) {
        return check;
    }
    refunded_amount_ = refunded_amount_ + amount;
    if (refunded_amount_ >= amount_) {
        refund_status_ = RefundStatus::COMPLETED;
        status_ = TransactionStatus::REFUNDED;
    } else {
        refund_status_ = RefundStatus::COMPLETED;
        status_ = TransactionStatus::PARTIALLY_REFUNDED;
    }
    return Result::ok();
}

std::string Transaction::to_string() const {
    std::ostringstream oss;
    oss << "Transaction(id=" << id_
        << ", order_id=" << order_id_
        << ", amount=" << amount_
        << ", status=" << static_cast<int>(status_)
        << ")";
    return oss.str();
}

RefundRequest::RefundRequest()
    : id_(0), transaction_id_(0), order_id_(0), amount_(0),
      status_(RefundStatus::REQUESTED), requested_by_(0),
      requested_at_(time(nullptr)), processed_at_(0), is_partial_(false) {
}

RefundRequest::RefundRequest(RefundId id, TransactionId transaction_id, Money amount)
    : id_(id), transaction_id_(transaction_id), order_id_(0), amount_(amount),
      status_(RefundStatus::REQUESTED), requested_by_(0),
      requested_at_(time(nullptr)), processed_at_(0), is_partial_(false) {
}

AlipayGateway::AlipayGateway()
    : name_("Alipay"), fee_rate_(0.006), min_fee_(0), max_fee_(0), is_sandbox_(true) {
}

ResultT<Transaction*> AlipayGateway::process_payment(Transaction* txn) {
    if (!is_available()) {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason("Gateway unavailable");
        return ResultT<Transaction*>::error(ErrorCode::PAYMENT_GATEWAY_ERROR, "Alipay gateway unavailable");
    }

    if (txn->amount() < minimum_amount()) {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason("Amount below minimum");
        return ResultT<Transaction*>::error(ErrorCode::PAYMENT_INVALID_AMOUNT, "Amount below minimum");
    }

    txn->set_fee(calculate_fee(txn->amount()));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::ostringstream oss;
    oss << "ALIPAY_" << dis(gen) << "_" << time(nullptr);
    txn->set_gateway_transaction_id(oss.str());

    if (dis(gen) % 20 == 0) {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason("Random payment failure for testing");
        return ResultT<Transaction*>::ok(txn);
    }

    txn->set_status(TransactionStatus::SUCCESS);
    txn->set_paid_at(time(nullptr));

    return ResultT<Transaction*>::ok(txn);
}

ResultT<RefundRequest*> AlipayGateway::process_refund(RefundRequest* refund) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::ostringstream oss;
    oss << "ALIPAY_REFUND_" << dis(gen);
    refund->set_gateway_refund_id(oss.str());
    refund->set_status(RefundStatus::COMPLETED);
    refund->set_processed_at(time(nullptr));

    return ResultT<RefundRequest*>::ok(refund);
}

Result AlipayGateway::query_status(Transaction* txn) {
    (void)txn;
    return Result::ok();
}

bool AlipayGateway::is_available() const {
    return true;
}

Money AlipayGateway::calculate_fee(Money amount) const {
    Money fee = static_cast<Money>(amount * fee_rate_);
    if (min_fee_ > 0 && fee < min_fee_) return min_fee_;
    if (max_fee_ > 0 && fee > max_fee_) return max_fee_;
    return fee;
}

Money AlipayGateway::minimum_amount() const {
    return 1;
}

Money AlipayGateway::maximum_amount() const {
    return 50000000;
}

bool AlipayGateway::supports_currency(const std::string& currency) const {
    return currency == "CNY";
}

bool AlipayGateway::supports_method(PaymentMethod method) const {
    return method == PaymentMethod::ALIPAY || method == PaymentMethod::QR_CODE;
}

WechatPayGateway::WechatPayGateway()
    : name_("WeChat Pay"), fee_rate_(0.006) {
}

ResultT<Transaction*> WechatPayGateway::process_payment(Transaction* txn) {
    if (!is_available()) {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason("Gateway unavailable");
        return ResultT<Transaction*>::error(ErrorCode::PAYMENT_GATEWAY_ERROR, "WeChat Pay gateway unavailable");
    }

    txn->set_fee(calculate_fee(txn->amount()));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::ostringstream oss;
    oss << "WXPAY_" << dis(gen) << "_" << time(nullptr);
    txn->set_gateway_transaction_id(oss.str());

    if (dis(gen) % 20 == 0) {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason("Random payment failure for testing");
        return ResultT<Transaction*>::ok(txn);
    }

    txn->set_status(TransactionStatus::SUCCESS);
    txn->set_paid_at(time(nullptr));

    return ResultT<Transaction*>::ok(txn);
}

ResultT<RefundRequest*> WechatPayGateway::process_refund(RefundRequest* refund) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::ostringstream oss;
    oss << "WXPAY_REFUND_" << dis(gen);
    refund->set_gateway_refund_id(oss.str());
    refund->set_status(RefundStatus::COMPLETED);
    refund->set_processed_at(time(nullptr));

    return ResultT<RefundRequest*>::ok(refund);
}

Result WechatPayGateway::query_status(Transaction* txn) {
    (void)txn;
    return Result::ok();
}

bool WechatPayGateway::is_available() const {
    return true;
}

Money WechatPayGateway::calculate_fee(Money amount) const {
    return static_cast<Money>(amount * fee_rate_);
}

Money WechatPayGateway::minimum_amount() const {
    return 1;
}

Money WechatPayGateway::maximum_amount() const {
    return 5000000;
}

bool WechatPayGateway::supports_currency(const std::string& currency) const {
    return currency == "CNY";
}

bool WechatPayGateway::supports_method(PaymentMethod method) const {
    return method == PaymentMethod::WECHAT_PAY || method == PaymentMethod::MINI_PROGRAM;
}

CreditCardGateway::CreditCardGateway()
    : name_("Credit Card"), fee_rate_(0.029) {
    card_brand_rates_["VISA"] = 0.029;
    card_brand_rates_["MASTERCARD"] = 0.029;
    card_brand_rates_["AMEX"] = 0.035;
    card_brand_rates_["UNIONPAY"] = 0.015;
}

ResultT<Transaction*> CreditCardGateway::process_payment(Transaction* txn) {
    txn->set_fee(calculate_fee(txn->amount()));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::ostringstream oss;
    oss << "CC_" << dis(gen) << "_" << time(nullptr);
    txn->set_gateway_transaction_id(oss.str());

    if (dis(gen) % 15 == 0) {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason("Card declined");
        return ResultT<Transaction*>::ok(txn);
    }

    txn->set_status(TransactionStatus::SUCCESS);
    txn->set_paid_at(time(nullptr));

    return ResultT<Transaction*>::ok(txn);
}

ResultT<RefundRequest*> CreditCardGateway::process_refund(RefundRequest* refund) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::ostringstream oss;
    oss << "CC_REFUND_" << dis(gen);
    refund->set_gateway_refund_id(oss.str());
    refund->set_status(RefundStatus::COMPLETED);
    refund->set_processed_at(time(nullptr));

    return ResultT<RefundRequest*>::ok(refund);
}

Result CreditCardGateway::query_status(Transaction* txn) {
    (void)txn;
    return Result::ok();
}

bool CreditCardGateway::is_available() const {
    return true;
}

Money CreditCardGateway::calculate_fee(Money amount) const {
    return static_cast<Money>(amount * fee_rate_);
}

Money CreditCardGateway::minimum_amount() const {
    return 100;
}

Money CreditCardGateway::maximum_amount() const {
    return 10000000;
}

bool CreditCardGateway::supports_currency(const std::string& currency) const {
    return currency == "CNY" || currency == "USD" || currency == "EUR";
}

bool CreditCardGateway::supports_method(PaymentMethod method) const {
    return method == PaymentMethod::CREDIT_CARD || method == PaymentMethod::DEBIT_CARD;
}

Result CreditCardGateway::validate_card(const std::string& card_number, const std::string& expiry, const std::string& cvv) {
    if (card_number.length() < 13 || card_number.length() > 19) {
        return Result::error(ErrorCode::PAYMENT_INVALID_CARD, "Invalid card number length");
    }
    if (expiry.length() != 4) {
        return Result::error(ErrorCode::PAYMENT_INVALID_CARD, "Invalid expiry format");
    }
    if (cvv.length() < 3 || cvv.length() > 4) {
        return Result::error(ErrorCode::PAYMENT_INVALID_CARD, "Invalid CVV");
    }
    return Result::ok();
}

PaymentManager& PaymentManager::instance() {
    static PaymentManager instance;
    return instance;
}

PaymentManager::PaymentManager() {
}

PaymentManager::~PaymentManager() {
}

Result PaymentManager::init() {
    auto alipay = std::make_unique<AlipayGateway>();
    auto wechat = std::make_unique<WechatPayGateway>();
    auto credit_card = std::make_unique<CreditCardGateway>();

    gateways_[GatewayType::ALIPAY] = std::move(alipay);
    gateways_[GatewayType::WECHAT_PAY] = std::move(wechat);
    gateways_[GatewayType::CREDIT_CARD] = std::move(credit_card);

    default_gateways_[PaymentMethod::ALIPAY] = GatewayType::ALIPAY;
    default_gateways_[PaymentMethod::WECHAT_PAY] = GatewayType::WECHAT_PAY;
    default_gateways_[PaymentMethod::CREDIT_CARD] = GatewayType::CREDIT_CARD;
    default_gateways_[PaymentMethod::DEBIT_CARD] = GatewayType::CREDIT_CARD;
    default_gateways_[PaymentMethod::QR_CODE] = GatewayType::ALIPAY;
    default_gateways_[PaymentMethod::MINI_PROGRAM] = GatewayType::WECHAT_PAY;

    return Result::ok();
}

void PaymentManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    transactions_.clear();
    refunds_.clear();
    gateways_.clear();
    order_transaction_index_.clear();
    user_transaction_index_.clear();
}

ResultT<Transaction*> PaymentManager::create_transaction(OrderId order_id, UserId user_id,
                                                          Money amount, PaymentMethod method) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    TransactionId id = transactions_.size() + 1;
    auto txn = std::make_unique<Transaction>(id, order_id, amount, method);
    txn->set_user_id(user_id);

    auto it = default_gateways_.find(method);
    if (it != default_gateways_.end()) {
        txn->set_gateway(it->second);
    }

    auto txn_ptr = txn.get();
    transactions_[id] = std::move(txn);

    order_transaction_index_[order_id].push_back(id);
    user_transaction_index_[user_id].push_back(id);

    return ResultT<Transaction*>::ok(txn_ptr);
}

ResultT<Transaction*> PaymentManager::get_transaction(TransactionId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = transactions_.find(id);
    if (it == transactions_.end()) {
        return ResultT<Transaction*>::error(ErrorCode::PAYMENT_NOT_FOUND, "Transaction not found");
    }
    return ResultT<Transaction*>::ok(it->second.get());
}

ResultT<std::vector<Transaction*>> PaymentManager::get_transactions_by_order(OrderId order_id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Transaction*> result;
    auto it = order_transaction_index_.find(order_id);
    if (it != order_transaction_index_.end()) {
        for (TransactionId id : it->second) {
            result.push_back(transactions_[id].get());
        }
    }
    return ResultT<std::vector<Transaction*>>::ok(result);
}

ResultT<std::vector<Transaction*>> PaymentManager::get_transactions_by_user(UserId user_id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Transaction*> result;
    auto it = user_transaction_index_.find(user_id);
    if (it != user_transaction_index_.end()) {
        for (TransactionId id : it->second) {
            result.push_back(transactions_[id].get());
        }
    }
    return ResultT<std::vector<Transaction*>>::ok(result);
}

ResultT<std::vector<Transaction*>> PaymentManager::list_transactions(size_t page, size_t page_size) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Transaction*> result;
    size_t start = page * page_size;
    size_t end = start + page_size;
    size_t index = 0;

    for (const auto& pair : transactions_) {
        if (index >= start && index < end) {
            result.push_back(pair.second.get());
        }
        index++;
        if (index >= end) {
            break;
        }
    }

    return ResultT<std::vector<Transaction*>>::ok(result);
}

Result PaymentManager::process_payment(TransactionId transaction_id) {
    auto txn_result = get_transaction(transaction_id);
    if (!txn_result) {
        return Result::error(txn_result.error_code(), txn_result.error_message());
    }

    Transaction* txn = txn_result.value();
    PaymentGateway* gateway = get_gateway(txn->gateway());

    if (!gateway) {
        return Result::error(ErrorCode::PAYMENT_GATEWAY_NOT_FOUND, "Payment gateway not found");
    }

    if (!gateway->supports_method(txn->method())) {
        return Result::error(ErrorCode::PAYMENT_METHOD_NOT_SUPPORTED, "Payment method not supported by gateway");
    }

    auto process_result = gateway->process_payment(txn);
    if (!process_result) {
        return Result::error(process_result.error_code(), process_result.error_message());
    }

    return Result::ok();
}

Result PaymentManager::cancel_payment(TransactionId transaction_id) {
    auto txn_result = get_transaction(transaction_id);
    if (!txn_result) {
        return Result::error(txn_result.error_code(), txn_result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    Transaction* txn = txn_result.value();

    if (txn->status() != TransactionStatus::PENDING && txn->status() != TransactionStatus::PROCESSING) {
        return Result::error(ErrorCode::PAYMENT_INVALID_STATUS, "Only pending or processing payments can be cancelled");
    }

    txn->set_status(TransactionStatus::CANCELLED);
    return Result::ok();
}

ResultT<RefundRequest*> PaymentManager::create_refund(TransactionId transaction_id, Money amount,
                                                       const std::string& reason, UserId requested_by) {
    auto txn_result = get_transaction(transaction_id);
    if (!txn_result) {
        return ResultT<RefundRequest*>::error(txn_result.error_code(), txn_result.error_message());
    }

    Transaction* txn = txn_result.value();
    Result can_refund = txn->can_refund(amount);
    if (!can_refund) {
        return ResultT<RefundRequest*>::error(can_refund.error_code(), can_refund.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);

    RefundId id = refunds_.size() + 1;
    auto refund = std::make_unique<RefundRequest>(id, transaction_id, amount);
    refund->set_reason(reason);
    refund->set_requested_by(requested_by);
    refund->set_order_id(txn->order_id());
    refund->set_partial(amount < txn->amount());

    auto refund_ptr = refund.get();
    refunds_[id] = std::move(refund);

    return ResultT<RefundRequest*>::ok(refund_ptr);
}

ResultT<RefundRequest*> PaymentManager::get_refund(RefundId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = refunds_.find(id);
    if (it == refunds_.end()) {
        return ResultT<RefundRequest*>::error(ErrorCode::PAYMENT_REFUND_NOT_FOUND, "Refund not found");
    }
    return ResultT<RefundRequest*>::ok(it->second.get());
}

Result PaymentManager::process_refund(RefundId refund_id) {
    auto refund_result = get_refund(refund_id);
    if (!refund_result) {
        return Result::error(refund_result.error_code(), refund_result.error_message());
    }

    RefundRequest* refund = refund_result.value();

    auto txn_result = get_transaction(refund->transaction_id());
    if (!txn_result) {
        return Result::error(txn_result.error_code(), txn_result.error_message());
    }

    Transaction* txn = txn_result.value();
    PaymentGateway* gateway = get_gateway(txn->gateway());

    if (!gateway) {
        return Result::error(ErrorCode::PAYMENT_GATEWAY_NOT_FOUND, "Payment gateway not found");
    }

    auto process_result = gateway->process_refund(refund);
    if (!process_result) {
        return Result::error(process_result.error_code(), process_result.error_message());
    }

    if (refund->status() == RefundStatus::COMPLETED) {
        txn->record_refund(refund->amount());
    }

    return Result::ok();
}

Result PaymentManager::reject_refund(RefundId refund_id, const std::string& reason) {
    auto refund_result = get_refund(refund_id);
    if (!refund_result) {
        return Result::error(refund_result.error_code(), refund_result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    RefundRequest* refund = refund_result.value();
    refund->set_status(RefundStatus::REJECTED);
    refund->set_reject_reason(reason);
    refund->set_processed_at(time(nullptr));

    return Result::ok();
}

Result PaymentManager::register_gateway(std::unique_ptr<PaymentGateway> gateway) {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    gateways_[gateway->type()] = std::move(gateway);
    return Result::ok();
}

PaymentGateway* PaymentManager::get_gateway(GatewayType type) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = gateways_.find(type);
    if (it == gateways_.end()) {
        return nullptr;
    }
    return it->second.get();
}

PaymentGateway* PaymentManager::get_default_gateway(PaymentMethod method) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = default_gateways_.find(method);
    if (it == default_gateways_.end()) {
        return nullptr;
    }
    return get_gateway(it->second);
}

ResultT<Money> PaymentManager::calculate_total_fees(const TimeRange& range) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    Money total = 0;
    for (const auto& pair : transactions_) {
        const auto& txn = pair.second;
        auto txn_time = std::chrono::system_clock::from_time_t(txn->created_at());
        if (txn->is_success() && txn_time >= range.start && txn_time <= range.end) {
            total = total + txn->fee();
        }
    }
    return ResultT<Money>::ok(total);
}

ResultT<Money> PaymentManager::calculate_total_volume(const TimeRange& range) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    Money total = 0;
    for (const auto& pair : transactions_) {
        const auto& txn = pair.second;
        auto txn_time = std::chrono::system_clock::from_time_t(txn->created_at());
        if (txn->is_success() && txn_time >= range.start && txn_time <= range.end) {
            total = total + txn->amount();
        }
    }
    return ResultT<Money>::ok(total);
}

ResultT<size_t> PaymentManager::get_success_count(const TimeRange& range) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& pair : transactions_) {
        const auto& txn = pair.second;
        auto txn_time = std::chrono::system_clock::from_time_t(txn->created_at());
        if (txn->is_success() && txn_time >= range.start && txn_time <= range.end) {
            count++;
        }
    }
    return ResultT<size_t>::ok(count);
}

ResultT<size_t> PaymentManager::get_failure_count(const TimeRange& range) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& pair : transactions_) {
        const auto& txn = pair.second;
        auto txn_time = std::chrono::system_clock::from_time_t(txn->created_at());
        if (txn->is_failed() && txn_time >= range.start && txn_time <= range.end) {
            count++;
        }
    }
    return ResultT<size_t>::ok(count);
}

Result PaymentManager::retry_failed_transaction(TransactionId id) {
    auto txn_result = get_transaction(id);
    if (!txn_result) {
        return Result::error(txn_result.error_code(), txn_result.error_message());
    }

    Transaction* txn = txn_result.value();
    if (!txn->is_failed()) {
        return Result::error(ErrorCode::PAYMENT_INVALID_STATUS, "Only failed transactions can be retried");
    }

    txn->set_status(TransactionStatus::PENDING);
    txn->set_failure_reason("");

    return process_payment(id);
}

ResultT<std::vector<Transaction*>> PaymentManager::get_pending_transactions() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Transaction*> result;
    for (const auto& pair : transactions_) {
        if (pair.second->is_pending()) {
            result.push_back(pair.second.get());
        }
    }
    return ResultT<std::vector<Transaction*>>::ok(result);
}

Result PaymentManager::update_transaction_status(TransactionId id, TransactionStatus status) {
    auto txn_result = get_transaction(id);
    if (!txn_result) {
        return Result::error(txn_result.error_code(), txn_result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    txn_result.value()->set_status(status);
    return Result::ok();
}

Result PaymentManager::batch_process_payments(const std::vector<TransactionId>& ids) {
    for (size_t i = 0; i < ids.size(); i++) {
        process_payment(ids[i]);
    }
    return Result::ok();
}

Result PaymentManager::batch_process_refunds(const std::vector<RefundId>& ids) {
    int success_count = 0;
    for (size_t i = 0; i < ids.size(); i++) {
        Result r = process_refund(ids[i]);
        if (r) {
            success_count++;
        }
    }
    return Result::ok();
}

ResultT<double> PaymentManager::calculate_refund_rate(const TimeRange& range) {
    auto success_result = get_success_count(range);
    auto failure_result = get_failure_count(range);

    size_t success = success_result.value();
    size_t total = success + failure_result.value();

    double rate = total > 0 ? success / total : 0.0;
    return ResultT<double>::ok(rate);
}

ResultT<std::map<PaymentMethod, double>> PaymentManager::get_method_success_rates(const TimeRange& range) {
    std::map<PaymentMethod, std::pair<size_t, size_t>> stats;

    for (const auto& pair : transactions_) {
        const auto& txn = pair.second;
        auto txn_time = std::chrono::system_clock::from_time_t(txn->created_at());
        if (txn_time >= range.start && txn_time <= range.end) {
            stats[txn->method()].first++;
            if (txn->is_success()) {
                stats[txn->method()].second++;
            }
        }
    }

    std::map<PaymentMethod, double> result;
    for (const auto& pair : stats) {
        result[pair.first] = pair.second.second / pair.second.first;
    }

    return ResultT<std::map<PaymentMethod, double>>::ok(result);
}

Result PaymentManager::webhook_callback(const std::string& event_type, const std::string& payload) {
    if (event_type == "payment.success") {
        size_t pos = payload.find("transaction_id=");
        if (pos != std::string::npos) {
            TransactionId id = std::stoull(payload.substr(pos + 15));
            auto result = get_transaction(id);
            if (result) {
                result.value()->set_status(TransactionStatus::SUCCESS);
                pending_callbacks_[id] = payload;
            }
        }
    } else if (event_type == "payment.failed") {
        size_t pos = payload.find("transaction_id=");
        if (pos != std::string::npos) {
            TransactionId id = std::stoull(payload.substr(pos + 15));
            process_payment_callback(id, false, payload);
        }
    }
    return Result::ok();
}

void PaymentManager::process_payment_callback(TransactionId id, bool success, const std::string& gateway_response) {
    auto txn_result = get_transaction(id);
    if (!txn_result) return;

    Transaction* txn = txn_result.value();
    if (success) {
        txn->set_status(TransactionStatus::SUCCESS);
    } else {
        txn->set_status(TransactionStatus::FAILED);
        txn->set_failure_reason(gateway_response);
    }
}

} // namespace payment
} // namespace oms
