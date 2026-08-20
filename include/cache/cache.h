#ifndef CACHE_CACHE_H
#define CACHE_CACHE_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <functional>
#include <chrono>
#include <optional>
#include <list>
#include <algorithm>
#include <climits>

namespace oms {
namespace cache {

enum class CacheStrategy {
    LRU = 1,
    LFU = 2,
    FIFO = 3,
    RANDOM = 4,
    TTL_ONLY = 5
};

enum class CacheTier {
    MEMORY = 1,
    REDIS = 2,
    MEMORY_REDIS = 3
};

class CacheEntry {
public:
    CacheEntry();
    CacheEntry(const std::string& key, const std::string& value, int ttl_seconds = 3600);

    const std::string& key() const { return key_; }
    const std::string& value() const { return value_; }
    void set_value(const std::string& value) { value_ = value; }

    time_t created_at() const { return created_at_; }
    time_t expires_at() const { return expires_at_; }
    time_t last_accessed() const { return last_accessed_; }

    int access_count() const { return access_count_; }
    void record_access() {
        last_accessed_ = time(nullptr);
        access_count_++;
    }

    bool is_expired() const {
        return time(nullptr) > expires_at_;
    }

    int ttl() const {
        time_t now = time(nullptr);
        if (now > expires_at_) return 0;
        return static_cast<int>(expires_at_ - now);
    }

    void set_ttl(int seconds) {
        expires_at_ = time(nullptr) + seconds;
    }

    size_t size() const { return key_.size() + value_.size(); }

private:
    std::string key_;
    std::string value_;
    time_t created_at_;
    time_t expires_at_;
    time_t last_accessed_;
    int access_count_;
};

class CacheStatistics {
public:
    CacheStatistics();

    uint64_t hits() const { return hits_; }
    uint64_t misses() const { return misses_; }
    uint64_t evictions() const { return evictions_; }
    uint64_t sets() const { return sets_; }
    uint64_t deletes() const { return deletes_; }

    double hit_rate() const {
        uint64_t total = hits_ + misses_;
        if (total == 0) return 0.0;
        return static_cast<double>(hits_) / total;
    }

    void record_hit() { hits_++; }
    void record_miss() { misses_++; }
    void record_eviction() { evictions_++; }
    void record_set() { sets_++; }
    void record_delete() { deletes_++; }

    void reset();

    std::string to_string() const;

private:
    uint64_t hits_;
    uint64_t misses_;
    uint64_t evictions_;
    uint64_t sets_;
    uint64_t deletes_;
};

class CacheBackend {
public:
    virtual ~CacheBackend() = default;

    virtual const std::string& name() const = 0;

    virtual ResultT<std::optional<std::string>> get(const std::string& key) = 0;
    virtual Result set(const std::string& key, const std::string& value, int ttl_seconds = 0) = 0;
    virtual Result del(const std::string& key) = 0;
    virtual ResultT<bool> exists(const std::string& key) = 0;

    virtual Result clear() = 0;

    virtual size_t size() const = 0;
    virtual size_t max_size() const = 0;

    virtual const CacheStatistics& statistics() const = 0;
    virtual void reset_statistics() = 0;

    virtual bool is_available() const = 0;

    virtual ResultT<std::vector<std::string>> keys(const std::string& pattern = "*") = 0;
};

class MemoryCache : public CacheBackend {
public:
    MemoryCache(size_t max_memory_bytes, CacheStrategy strategy = CacheStrategy::LRU);
    ~MemoryCache() override;

    const std::string& name() const override { return name_; }

    ResultT<std::optional<std::string>> get(const std::string& key) override;
    Result set(const std::string& key, const std::string& value, int ttl_seconds = 0) override;
    Result del(const std::string& key) override;
    ResultT<bool> exists(const std::string& key) override;

    Result clear() override;

    size_t size() const override;
    size_t max_size() const override { return max_memory_bytes_; }

    const CacheStatistics& statistics() const override { return stats_; }
    void reset_statistics() override;

    bool is_available() const override { return true; }

    ResultT<std::vector<std::string>> keys(const std::string& pattern = "*") override;

    void set_strategy(CacheStrategy strategy) { strategy_ = strategy; }
    CacheStrategy strategy() const { return strategy_; }

    size_t memory_usage() const { return current_memory_bytes_; }
    double memory_usage_percent() const {
        if (max_memory_bytes_ == 0) return 0.0;
        return static_cast<double>(current_memory_bytes_) / max_memory_bytes_ * 100.0;
    }

    void cleanup_expired();
    size_t cleanup_count() const;

private:
    void evict_if_needed();
    void evict_one();

    std::string name_;
    size_t max_memory_bytes_;
    size_t current_memory_bytes_;
    CacheStrategy strategy_;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<CacheEntry>> entries_;
    std::list<std::string> access_order_;
    std::unordered_map<std::string, std::list<std::string>::iterator> access_order_index_;

    CacheStatistics stats_;
};

class RedisConfig {
public:
    RedisConfig();

    const std::string& host() const { return host_; }
    void set_host(const std::string& host) { host_ = host; }

    int port() const { return port_; }
    void set_port(int port) { port_ = port; }

    const std::string& password() const { return password_; }
    void set_password(const std::string& password) { password_ = password; }

    int database() const { return database_; }
    void set_database(int db) { database_ = db; }

    const std::string& key_prefix() const { return key_prefix_; }
    void set_key_prefix(const std::string& prefix) { key_prefix_ = prefix; }

    int connect_timeout() const { return connect_timeout_; }
    void set_connect_timeout(int ms) { connect_timeout_ = ms; }

    int socket_timeout() const { return socket_timeout_; }
    void set_socket_timeout(int ms) { socket_timeout_ = ms; }

    int max_connections() const { return max_connections_; }
    void set_max_connections(int max) { max_connections_ = max; }

    bool use_ssl() const { return use_ssl_; }
    void set_use_ssl(bool ssl) { use_ssl_ = ssl; }

private:
    std::string host_;
    int port_;
    std::string password_;
    int database_;
    std::string key_prefix_;
    int connect_timeout_;
    int socket_timeout_;
    int max_connections_;
    bool use_ssl_;
};

class RedisCache : public CacheBackend {
public:
    RedisCache(const RedisConfig& config);
    ~RedisCache() override;

    Result connect();
    void disconnect();
    bool is_connected() const { return connected_; }

    const std::string& name() const override { return name_; }

    ResultT<std::optional<std::string>> get(const std::string& key) override;
    Result set(const std::string& key, const std::string& value, int ttl_seconds = 0) override;
    Result del(const std::string& key) override;
    ResultT<bool> exists(const std::string& key) override;

    Result clear() override;

    size_t size() const override;
    size_t max_size() const override { return SIZE_MAX; }

    const CacheStatistics& statistics() const override { return stats_; }
    void reset_statistics() override;

    bool is_available() const override { return connected_; }

    ResultT<std::vector<std::string>> keys(const std::string& pattern = "*") override;

    Result expire(const std::string& key, int seconds);
    ResultT<int> ttl(const std::string& key);

    ResultT<size_t> increment(const std::string& key, int amount = 1);
    ResultT<size_t> decrement(const std::string& key, int amount = 1);

    ResultT<bool> set_nx(const std::string& key, const std::string& value, int ttl_seconds = 0);

    const RedisConfig& config() const { return config_; }

private:
    std::string prefixed_key(const std::string& key) const;

    std::string name_;
    RedisConfig config_;
    bool connected_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<CacheEntry>> entries_;
    CacheStatistics stats_;
};

class MultiTierCache : public CacheBackend {
public:
    MultiTierCache();
    ~MultiTierCache() override;

    const std::string& name() const override { return name_; }

    void add_backend(std::shared_ptr<CacheBackend> backend);

    ResultT<std::optional<std::string>> get(const std::string& key) override;
    Result set(const std::string& key, const std::string& value, int ttl_seconds = 0) override;
    Result del(const std::string& key) override;
    ResultT<bool> exists(const std::string& key) override;

    Result clear() override;

    size_t size() const override;
    size_t max_size() const override;

    const CacheStatistics& statistics() const override { return stats_; }
    void reset_statistics() override;

    bool is_available() const override;

    ResultT<std::vector<std::string>> keys(const std::string& pattern = "*") override;

    void set_write_through(bool enabled) { write_through_ = enabled; }
    bool write_through() const { return write_through_; }

    void set_read_through(bool enabled) { read_through_ = enabled; }
    bool read_through() const { return read_through_; }

    const std::vector<std::shared_ptr<CacheBackend>>& backends() const { return backends_; }

private:
    std::string name_;
    std::vector<std::shared_ptr<CacheBackend>> backends_;
    bool write_through_;
    bool read_through_;
    CacheStatistics stats_;
};

class CacheManager {
public:
    static CacheManager& instance();

    Result init(CacheTier tier = CacheTier::MEMORY,
                size_t memory_limit = 1024 * 1024 * 64,
                const RedisConfig* redis_config = nullptr);
    void shutdown();

    ResultT<std::optional<std::string>> get(const std::string& key);
    Result set(const std::string& key, const std::string& value, int ttl_seconds = 3600);
    Result del(const std::string& key);
    ResultT<bool> exists(const std::string& key);

    Result clear();

    template<typename T>
    ResultT<std::optional<T>> get_object(const std::string& key) {
        auto result = get(key);
        if (!result) {
            return ResultT<std::optional<T>>::error(result.error_code(), result.error_message());
        }
        if (!result.value()) {
            return ResultT<std::optional<T>>::ok(std::nullopt);
        }
        return ResultT<std::optional<T>>::ok(T{});
    }

    template<typename T>
    Result set_object(const std::string& key, const T& value, int ttl_seconds = 3600) {
        (void)value;
        return set(key, "serialized_object", ttl_seconds);
    }

    CacheBackend* backend() { return backend_.get(); }
    const CacheBackend* backend() const { return backend_.get(); }

    const CacheStatistics& statistics() const;
    void reset_statistics();

    bool is_available() const;

    size_t size() const;

    ResultT<std::vector<std::string>> keys(const std::string& pattern = "*");

    void cleanup();

private:
    CacheManager();
    ~CacheManager();

    std::shared_ptr<CacheBackend> backend_;
    CacheTier tier_;
    mutable std::shared_mutex mutex_;
};

class CacheLock {
public:
    CacheLock(CacheBackend& backend, const std::string& key, int timeout_ms = 1000);
    ~CacheLock();

    bool acquired() const { return acquired_; }
    bool try_acquire(int timeout_ms = 1000);
    void release();

private:
    CacheBackend& backend_;
    std::string key_;
    bool acquired_;
};

} // namespace cache
} // namespace oms

#endif // CACHE_CACHE_H
