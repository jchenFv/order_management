#ifndef DB_DATABASE_H
#define DB_DATABASE_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <shared_mutex>
#include <functional>
#include <any>
#include <variant>
#include <condition_variable>

namespace oms {
namespace db {

enum class ConnectionState {
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    RECONNECTING = 3,
    FAILED = 4
};

enum class IsolationLevel {
    READ_UNCOMMITTED = 1,
    READ_COMMITTED = 2,
    REPEATABLE_READ = 3,
    SERIALIZABLE = 4
};

class DatabaseConfig {
public:
    DatabaseConfig();

    const std::string& host() const { return host_; }
    void set_host(const std::string& host) { host_ = host; }

    int port() const { return port_; }
    void set_port(int port) { port_ = port; }

    const std::string& database() const { return database_; }
    void set_database(const std::string& db) { database_ = db; }

    const std::string& username() const { return username_; }
    void set_username(const std::string& user) { username_ = user; }

    const std::string& password() const { return password_; }
    void set_password(const std::string& pass) { password_ = pass; }

    const std::string& charset() const { return charset_; }
    void set_charset(const std::string& cs) { charset_ = cs; }

    int max_connections() const { return max_connections_; }
    void set_max_connections(int max) { max_connections_ = max; }

    int connect_timeout() const { return connect_timeout_; }
    void set_connect_timeout(int seconds) { connect_timeout_ = seconds; }

    int read_timeout() const { return read_timeout_; }
    void set_read_timeout(int seconds) { read_timeout_ = seconds; }

    int write_timeout() const { return write_timeout_; }
    void set_write_timeout(int seconds) { write_timeout_ = seconds; }

    bool use_ssl() const { return use_ssl_; }
    void set_use_ssl(bool ssl) { use_ssl_ = ssl; }

    bool auto_commit() const { return auto_commit_; }
    void set_auto_commit(bool auto_commit) { auto_commit_ = auto_commit; }

    IsolationLevel isolation_level() const { return isolation_level_; }
    void set_isolation_level(IsolationLevel level) { isolation_level_ = level; }

    std::string to_connection_string() const;

private:
    std::string host_;
    int port_;
    std::string database_;
    std::string username_;
    std::string password_;
    std::string charset_;
    int max_connections_;
    int connect_timeout_;
    int read_timeout_;
    int write_timeout_;
    bool use_ssl_;
    bool auto_commit_;
    IsolationLevel isolation_level_;
};

class QueryResult {
public:
    using Row = std::map<std::string, std::any>;
    using RowVector = std::vector<Row>;

    QueryResult();
    QueryResult(bool success, const std::string& error = "");

    bool success() const { return success_; }
    const std::string& error_message() const { return error_message_; }
    int affected_rows() const { return affected_rows_; }
    void set_affected_rows(int rows) { affected_rows_ = rows; }

    uint64_t last_insert_id() const { return last_insert_id_; }
    void set_last_insert_id(uint64_t id) { last_insert_id_ = id; }

    size_t row_count() const { return rows_.size(); }
    size_t column_count() const { return columns_.size(); }

    const std::vector<std::string>& columns() const { return columns_; }
    void set_columns(const std::vector<std::string>& cols) { columns_ = cols; }

    const RowVector& rows() const { return rows_; }
    void add_row(const Row& row) { rows_.push_back(row); }

    const Row& operator[](size_t index) const { return rows_[index]; }

    template<typename T>
    std::optional<T> get(size_t row, const std::string& column) const {
        if (row >= rows_.size()) {
            return std::nullopt;
        }
        auto it = rows_[row].find(column);
        if (it == rows_[row].end()) {
            return std::nullopt;
        }
        try {
            return std::any_cast<T>(it->second);
        } catch (...) {
            return std::nullopt;
        }
    }

    bool has_column(const std::string& column) const;

    void set_error(const std::string& error) {
        success_ = false;
        error_message_ = error;
    }

private:
    bool success_;
    std::string error_message_;
    int affected_rows_;
    uint64_t last_insert_id_;
    std::vector<std::string> columns_;
    RowVector rows_;
};

class QueryBuilder {
public:
    QueryBuilder();

    QueryBuilder& select(const std::vector<std::string>& columns);
    QueryBuilder& select_all();
    QueryBuilder& from(const std::string& table);
    QueryBuilder& where(const std::string& condition);
    QueryBuilder& where(const std::string& column, const std::string& op, const std::string& value);
    QueryBuilder& and_where(const std::string& condition);
    QueryBuilder& or_where(const std::string& condition);
    QueryBuilder& order_by(const std::string& column, bool ascending = true);
    QueryBuilder& group_by(const std::string& column);
    QueryBuilder& having(const std::string& condition);
    QueryBuilder& limit(size_t count);
    QueryBuilder& offset(size_t offset);
    QueryBuilder& join(const std::string& table, const std::string& condition);
    QueryBuilder& left_join(const std::string& table, const std::string& condition);
    QueryBuilder& right_join(const std::string& table, const std::string& condition);

    QueryBuilder& insert(const std::string& table);
    QueryBuilder& values(const std::map<std::string, std::string>& data);

    QueryBuilder& update(const std::string& table);
    QueryBuilder& set(const std::map<std::string, std::string>& data);

    QueryBuilder& delete_from(const std::string& table);

    QueryBuilder& raw(const std::string& sql);

    std::string build() const;
    void reset();

private:
    enum class QueryType {
        NONE,
        SELECT,
        INSERT,
        UPDATE,
        DELETE,
        RAW
    };

    QueryType type_;
    std::string table_;
    std::vector<std::string> columns_;
    std::vector<std::string> where_conditions_;
    std::vector<std::string> order_by_;
    std::vector<std::string> group_by_;
    std::string having_condition_;
    std::vector<std::string> joins_;
    size_t limit_;
    size_t offset_;
    std::map<std::string, std::string> data_;
    std::string raw_sql_;
};

class Connection {
public:
    Connection(const DatabaseConfig& config);
    ~Connection();

    Result connect();
    void disconnect();
    bool is_connected() const;
    ConnectionState state() const { return state_; }

    ResultT<QueryResult> execute(const std::string& sql);
    ResultT<QueryResult> execute_query(const QueryBuilder& builder);

    Result begin_transaction();
    Result commit();
    Result rollback();
    Result set_savepoint(const std::string& name);
    Result rollback_to_savepoint(const std::string& name);
    Result release_savepoint(const std::string& name);

    bool in_transaction() const { return in_transaction_; }

    int last_errno() const { return last_errno_; }
    const std::string& last_error() const { return last_error_; }

    void ping();
    time_t last_used() const { return last_used_; }

private:
    void reset_errors();

    DatabaseConfig config_;
    ConnectionState state_;
    bool in_transaction_;
    int last_errno_;
    std::string last_error_;
    time_t last_used_;
    int transaction_depth_;
    std::map<std::string, size_t> savepoints_;
};

class ConnectionPool {
public:
    ConnectionPool(const DatabaseConfig& config);
    ~ConnectionPool();

    Result init();
    void shutdown();

    std::shared_ptr<Connection> get_connection();
    void release_connection(std::shared_ptr<Connection> conn);

    size_t total_connections() const;
    size_t available_connections() const;
    size_t busy_connections() const;

    void set_max_connections(size_t max);
    size_t max_connections() const { return max_connections_; }

    Result wait_for_connection(int timeout_ms = 5000);

    const DatabaseConfig& config() const { return config_; }

    void check_connections();

private:
    std::shared_ptr<Connection> create_connection();

    DatabaseConfig config_;
    size_t max_connections_;
    size_t min_connections_;
    mutable std::shared_mutex mutex_;
    std::vector<std::shared_ptr<Connection>> available_;
    std::vector<std::shared_ptr<Connection>> busy_;
    std::condition_variable_any cv_;
    bool shutdown_;
};

class Transaction {
public:
    Transaction(ConnectionPool& pool);
    ~Transaction();

    Result begin();
    Result commit();
    Result rollback();

    ResultT<QueryResult> execute(const std::string& sql);
    ResultT<QueryResult> execute_query(const QueryBuilder& builder);

    bool is_active() const { return active_; }

    Result set_savepoint(const std::string& name);
    Result rollback_to_savepoint(const std::string& name);

private:
    ConnectionPool& pool_;
    std::shared_ptr<Connection> conn_;
    bool active_;
    bool committed_;
};

class Repository {
public:
    Repository(ConnectionPool& pool);
    virtual ~Repository() = default;

    ConnectionPool& pool() { return pool_; }

protected:
    ConnectionPool& pool_;
};

class DatabaseManager {
public:
    static DatabaseManager& instance();

    Result init(const DatabaseConfig& config);
    void shutdown();

    ConnectionPool& pool() { return *pool_; }

    ResultT<QueryResult> execute(const std::string& sql);
    ResultT<QueryResult> query(const std::string& sql);

    Transaction begin_transaction();

    bool is_healthy() const;

    std::string get_database_version() const;

    Result ping();

private:
    DatabaseManager();
    ~DatabaseManager();

    std::unique_ptr<ConnectionPool> pool_;
};

} // namespace db
} // namespace oms

#endif // DB_DATABASE_H
