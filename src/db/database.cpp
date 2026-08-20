#include "db/database.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <cassert>

namespace oms {
namespace db {

DatabaseConfig::DatabaseConfig()
    : host_("localhost"), port_(3306), database_("oms"),
      username_("root"), password_(""), charset_("utf8mb4"),
      max_connections_(10), connect_timeout_(10), read_timeout_(30),
      write_timeout_(30), use_ssl_(false), auto_commit_(true),
      isolation_level_(IsolationLevel::READ_COMMITTED) {
}

std::string DatabaseConfig::to_connection_string() const {
    std::ostringstream oss;
    oss << "host=" << host_
        << " port=" << port_
        << " db=" << database_
        << " user=" << username_
        << " password=" << password_
        << " charset=" << charset_;
    return oss.str();
}

QueryResult::QueryResult()
    : success_(true), affected_rows_(0), last_insert_id_(0) {
}

QueryResult::QueryResult(bool success, const std::string& error)
    : success_(success), error_message_(error),
      affected_rows_(0), last_insert_id_(0) {
}

bool QueryResult::has_column(const std::string& column) const {
    return std::find(columns_.begin(), columns_.end(), column) != columns_.end();
}

QueryBuilder::QueryBuilder()
    : type_(QueryType::NONE), limit_(0), offset_(0) {
}

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& columns) {
    type_ = QueryType::SELECT;
    columns_ = columns;
    return *this;
}

QueryBuilder& QueryBuilder::select_all() {
    type_ = QueryType::SELECT;
    columns_.clear();
    columns_.push_back("*");
    return *this;
}

QueryBuilder& QueryBuilder::from(const std::string& table) {
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& condition) {
    where_conditions_.clear();
    where_conditions_.push_back(condition);
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& column, const std::string& op, const std::string& value) {
    std::ostringstream oss;
    oss << column << " " << op << " " << value;
    return where(oss.str());
}

QueryBuilder& QueryBuilder::and_where(const std::string& condition) {
    where_conditions_.push_back(condition);
    return *this;
}

QueryBuilder& QueryBuilder::or_where(const std::string& condition) {
    where_conditions_.push_back("OR " + condition);
    return *this;
}

QueryBuilder& QueryBuilder::order_by(const std::string& column, bool ascending) {
    std::ostringstream oss;
    oss << column << (ascending ? " ASC" : " DESC");
    order_by_.push_back(oss.str());
    return *this;
}

QueryBuilder& QueryBuilder::group_by(const std::string& column) {
    group_by_.push_back(column);
    return *this;
}

QueryBuilder& QueryBuilder::having(const std::string& condition) {
    having_condition_ = condition;
    return *this;
}

QueryBuilder& QueryBuilder::limit(size_t count) {
    limit_ = count;
    return *this;
}

QueryBuilder& QueryBuilder::offset(size_t offset) {
    offset_ = offset;
    return *this;
}

QueryBuilder& QueryBuilder::join(const std::string& table, const std::string& condition) {
    std::ostringstream oss;
    oss << "JOIN " << table << " ON " << condition;
    joins_.push_back(oss.str());
    return *this;
}

QueryBuilder& QueryBuilder::left_join(const std::string& table, const std::string& condition) {
    std::ostringstream oss;
    oss << "LEFT JOIN " << table << " ON " << condition;
    joins_.push_back(oss.str());
    return *this;
}

QueryBuilder& QueryBuilder::right_join(const std::string& table, const std::string& condition) {
    std::ostringstream oss;
    oss << "RIGHT JOIN " << table << " ON " << condition;
    joins_.push_back(oss.str());
    return *this;
}

QueryBuilder& QueryBuilder::insert(const std::string& table) {
    type_ = QueryType::INSERT;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::values(const std::map<std::string, std::string>& data) {
    data_ = data;
    return *this;
}

QueryBuilder& QueryBuilder::update(const std::string& table) {
    type_ = QueryType::UPDATE;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::set(const std::map<std::string, std::string>& data) {
    data_ = data;
    return *this;
}

QueryBuilder& QueryBuilder::delete_from(const std::string& table) {
    type_ = QueryType::DELETE;
    table_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::raw(const std::string& sql) {
    type_ = QueryType::RAW;
    raw_sql_ = sql;
    return *this;
}

std::string QueryBuilder::build() const {
    std::ostringstream oss;

    switch (type_) {
        case QueryType::SELECT: {
            oss << "SELECT ";
            for (size_t i = 0; i < columns_.size(); i++) {
                if (i > 0) oss << ", ";
                oss << columns_[i];
            }
            oss << " FROM " << table_;

            for (const auto& join : joins_) {
                oss << " " << join;
            }

            if (!where_conditions_.empty()) {
                oss << " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); i++) {
                    if (i > 0 && where_conditions_[i].substr(0, 3) != "OR ") {
                        oss << " AND ";
                    } else if (i > 0) {
                        oss << " ";
                    }
                    oss << where_conditions_[i];
                }
            }

            if (!group_by_.empty()) {
                oss << " GROUP BY ";
                for (size_t i = 0; i < group_by_.size(); i++) {
                    if (i > 0) oss << ", ";
                    oss << group_by_[i];
                }
                if (!having_condition_.empty()) {
                    oss << " HAVING " << having_condition_;
                }
            }

            if (!order_by_.empty()) {
                oss << " ORDER BY ";
                for (size_t i = 0; i < order_by_.size(); i++) {
                    if (i > 0) oss << ", ";
                    oss << order_by_[i];
                }
            }

            if (limit_ > 0) {
                oss << " LIMIT " << limit_;
                if (offset_ > 0) {
                    oss << " OFFSET " << offset_;
                }
            }
            break;
        }
        case QueryType::INSERT: {
            oss << "INSERT INTO " << table_ << " (";
            bool first = true;
            for (const auto& pair : data_) {
                if (!first) oss << ", ";
                oss << pair.first;
                first = false;
            }
            oss << ") VALUES (";
            first = true;
            for (const auto& pair : data_) {
                if (!first) oss << ", ";
                oss << pair.second;
                first = false;
            }
            oss << ")";
            break;
        }
        case QueryType::UPDATE: {
            oss << "UPDATE " << table_ << " SET ";
            bool first = true;
            for (const auto& pair : data_) {
                if (!first) oss << ", ";
                oss << pair.first << " = " << pair.second;
                first = false;
            }
            if (!where_conditions_.empty()) {
                oss << " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); i++) {
                    if (i > 0 && where_conditions_[i].substr(0, 3) != "OR ") {
                        oss << " AND ";
                    } else if (i > 0) {
                        oss << " ";
                    }
                    oss << where_conditions_[i];
                }
            }
            break;
        }
        case QueryType::DELETE: {
            oss << "DELETE FROM " << table_;
            if (!where_conditions_.empty()) {
                oss << " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); i++) {
                    if (i > 0 && where_conditions_[i].substr(0, 3) != "OR ") {
                        oss << " AND ";
                    } else if (i > 0) {
                        oss << " ";
                    }
                    oss << where_conditions_[i];
                }
            }
            break;
        }
        case QueryType::RAW: {
            oss << raw_sql_;
            break;
        }
        default:
            break;
    }

    return oss.str();
}

void QueryBuilder::reset() {
    type_ = QueryType::NONE;
    table_.clear();
    columns_.clear();
    where_conditions_.clear();
    order_by_.clear();
    group_by_.clear();
    having_condition_.clear();
    joins_.clear();
    limit_ = 0;
    offset_ = 0;
    data_.clear();
    raw_sql_.clear();
}

Connection::Connection(const DatabaseConfig& config)
    : config_(config), state_(ConnectionState::DISCONNECTED),
      in_transaction_(false), last_errno_(0), last_used_(0),
      transaction_depth_(0) {
}

Connection::~Connection() {
    disconnect();
}

Result Connection::connect() {
    state_ = ConnectionState::CONNECTING;
    reset_errors();

    if (config_.host().empty()) {
        state_ = ConnectionState::FAILED;
        return Result::error(ErrorCode::DB_CONNECTION_FAILED, "Host cannot be empty");
    }

    state_ = ConnectionState::CONNECTED;
    last_used_ = time(nullptr);
    return Result::ok();
}

void Connection::disconnect() {
    if (in_transaction_) {
        rollback();
    }
    state_ = ConnectionState::DISCONNECTED;
}

bool Connection::is_connected() const {
    return state_ == ConnectionState::CONNECTED;
}

ResultT<QueryResult> Connection::execute(const std::string& sql) {
    reset_errors();

    if (!is_connected()) {
        Result reconnect = connect();
        if (!reconnect) {
            return ResultT<QueryResult>::error(reconnect.error_code(), reconnect.message());
        }
    }

    last_used_ = time(nullptr);

    QueryResult result(true);
    result.set_affected_rows(1);
    result.set_last_insert_id(1);

    return ResultT<QueryResult>::ok(result);
}

ResultT<QueryResult> Connection::execute_query(const QueryBuilder& builder) {
    return execute(builder.build());
}

Result Connection::begin_transaction() {
    if (in_transaction_) {
        transaction_depth_++;
        return Result::ok();
    }

    reset_errors();
    in_transaction_ = true;
    transaction_depth_ = 1;

    return Result::ok();
}

Result Connection::commit() {
    if (!in_transaction_) {
        return Result::error(ErrorCode::DB_TRANSACTION_ERROR, "No active transaction");
    }

    transaction_depth_--;
    if (transaction_depth_ == 0) {
        in_transaction_ = false;
        savepoints_.clear();
    }

    return Result::ok();
}

Result Connection::rollback() {
    if (!in_transaction_) {
        return Result::error(ErrorCode::DB_TRANSACTION_ERROR, "No active transaction");
    }

    transaction_depth_ = 0;
    in_transaction_ = false;
    savepoints_.clear();

    return Result::ok();
}

Result Connection::set_savepoint(const std::string& name) {
    if (!in_transaction_) {
        return Result::error(ErrorCode::DB_TRANSACTION_ERROR, "No active transaction");
    }

    savepoints_[name] = transaction_depth_;
    return Result::ok();
}

Result Connection::rollback_to_savepoint(const std::string& name) {
    auto it = savepoints_.find(name);
    if (it == savepoints_.end()) {
        return Result::error(ErrorCode::DB_TRANSACTION_ERROR, "Savepoint not found");
    }

    transaction_depth_ = it->second;
    return Result::ok();
}

Result Connection::release_savepoint(const std::string& name) {
    savepoints_.erase(name);
    return Result::ok();
}

void Connection::reset_errors() {
    last_errno_ = 0;
    last_error_.clear();
}

void Connection::ping() {
    last_used_ = time(nullptr);
}

ConnectionPool::ConnectionPool(const DatabaseConfig& config)
    : config_(config), max_connections_(config.max_connections()),
      min_connections_(1), shutdown_(false) {
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

Result ConnectionPool::init() {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    for (size_t i = 0; i < min_connections_; i++) {
        auto conn = create_connection();
        Result connect_result = conn->connect();
        if (!connect_result) {
            return connect_result;
        }
        available_.push_back(conn);
    }

    return Result::ok();
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    shutdown_ = true;

    for (auto& conn : available_) {
        conn->disconnect();
    }
    available_.clear();

    for (auto& conn : busy_) {
        conn->disconnect();
    }
    busy_.clear();

    cv_.notify_all();
}

std::shared_ptr<Connection> ConnectionPool::create_connection() {
    return std::make_shared<Connection>(config_);
}

std::shared_ptr<Connection> ConnectionPool::get_connection() {
    if (!available_.empty()) {
        auto conn = available_.back();
        available_.pop_back();
        busy_.push_back(conn);
        return conn;
    }

    auto conn = create_connection();
    Result connect_result = conn->connect();
    if (!connect_result) {
        return nullptr;
    }

    busy_.push_back(conn);
    return conn;
}

void ConnectionPool::release_connection(std::shared_ptr<Connection> conn) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = std::find(busy_.begin(), busy_.end(), conn);
    if (it != busy_.end()) {
        busy_.erase(it);
    }

    if (conn->is_connected() && available_.size() < max_connections_) {
        available_.push_back(conn);
    } else {
        conn->disconnect();
    }

    cv_.notify_one();
}

size_t ConnectionPool::total_connections() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return available_.size() + busy_.size();
}

size_t ConnectionPool::available_connections() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return available_.size();
}

size_t ConnectionPool::busy_connections() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return busy_.size();
}

void ConnectionPool::set_max_connections(size_t max) {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    max_connections_ = max;
}

Result ConnectionPool::wait_for_connection(int timeout_ms) {
    (void)timeout_ms;
    return Result::ok();
}

void ConnectionPool::check_connections() {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    time_t now = time(nullptr);
    const time_t max_idle = 300;

    auto it = available_.begin();
    while (it != available_.end()) {
        if (now - (*it)->last_used() > max_idle) {
            (*it)->disconnect();
            it = available_.erase(it);
        } else {
            ++it;
        }
    }
}

Transaction::Transaction(ConnectionPool& pool)
    : pool_(pool), active_(false), committed_(false) {
}

Transaction::~Transaction() {
    if (active_ && !committed_) {
        rollback();
    }
    if (conn_) {
        pool_.release_connection(conn_);
    }
}

Result Transaction::begin() {
    conn_ = pool_.get_connection();
    if (!conn_) {
        return Result::error(ErrorCode::DB_CONNECTION_FAILED, "Failed to get connection");
    }

    Result result = conn_->begin_transaction();
    if (result) {
        active_ = true;
    }
    return result;
}

Result Transaction::commit() {
    if (!active_) {
        return Result::error(ErrorCode::DB_TRANSACTION_ERROR, "No active transaction");
    }

    Result result = conn_->commit();
    if (result) {
        committed_ = true;
        active_ = false;
    }
    return result;
}

Result Transaction::rollback() {
    if (!active_) {
        return Result::error(ErrorCode::DB_TRANSACTION_ERROR, "No active transaction");
    }

    Result result = conn_->rollback();
    if (result) {
        active_ = false;
    }
    return result;
}

ResultT<QueryResult> Transaction::execute(const std::string& sql) {
    if (!conn_) {
        return ResultT<QueryResult>::error(ErrorCode::DB_CONNECTION_FAILED, "No connection");
    }
    return conn_->execute(sql);
}

ResultT<QueryResult> Transaction::execute_query(const QueryBuilder& builder) {
    return execute(builder.build());
}

Result Transaction::set_savepoint(const std::string& name) {
    if (!conn_) {
        return Result::error(ErrorCode::DB_CONNECTION_FAILED, "No connection");
    }
    return conn_->set_savepoint(name);
}

Result Transaction::rollback_to_savepoint(const std::string& name) {
    if (!conn_) {
        return Result::error(ErrorCode::DB_CONNECTION_FAILED, "No connection");
    }
    return conn_->rollback_to_savepoint(name);
}

Repository::Repository(ConnectionPool& pool)
    : pool_(pool) {
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
}

DatabaseManager::~DatabaseManager() {
}

Result DatabaseManager::init(const DatabaseConfig& config) {
    pool_ = std::make_unique<ConnectionPool>(config);
    return pool_->init();
}

void DatabaseManager::shutdown() {
    if (pool_) {
        pool_->shutdown();
    }
}

ResultT<QueryResult> DatabaseManager::execute(const std::string& sql) {
    auto conn = pool_->get_connection();
    if (!conn) {
        return ResultT<QueryResult>::error(ErrorCode::DB_CONNECTION_FAILED, "Failed to get connection");
    }

    auto result = conn->execute(sql);
    pool_->release_connection(conn);
    return result;
}

ResultT<QueryResult> DatabaseManager::query(const std::string& sql) {
    return execute(sql);
}

Transaction DatabaseManager::begin_transaction() {
    Transaction tx(*pool_);
    tx.begin();
    return tx;
}

bool DatabaseManager::is_healthy() const {
    return pool_ && pool_->total_connections() > 0;
}

std::string DatabaseManager::get_database_version() const {
    return "MySQL 8.0 (simulated)";
}

Result DatabaseManager::ping() {
    auto conn = pool_->get_connection();
    if (!conn) {
        return Result::error(ErrorCode::DB_CONNECTION_FAILED, "Failed to get connection");
    }
    conn->ping();
    pool_->release_connection(conn);
    return Result::ok();
}

} // namespace db
} // namespace oms
