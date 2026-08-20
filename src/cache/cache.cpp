#include "cache/cache.h"
#include <algorithm>
#include <sstream>
#include <random>
#include <ctime>
#include <cstring>

namespace oms {
namespace cache {

CacheEntry::CacheEntry()
    : created_at_(time(nullptr)), expires_at_(time(nullptr) + 3600),
      last_accessed_(time(nullptr)), access_count_(0) {
}

CacheEntry::CacheEntry(const std::string& key, const std::string& value, int ttl_seconds)
    : key_(key), value_(value), created_at_(time(nullptr)),
      last_accessed_(time(nullptr)), access_count_(0) {
    expires_at_ = time(nullptr) + ttl_seconds;
}

CacheStatistics::CacheStatistics()
    : hits_(0), misses_(0), evictions_(0), sets_(0), deletes_(0) {
}

void CacheStatistics::reset() {
    hits_ = 0;
    misses_ = 0;
    evictions_ = 0;
    sets_ = 0;
    deletes_ = 0;
}

std::string CacheStatistics::to_string() const {
    std::ostringstream oss;
    oss << "CacheStatistics(hits=" << hits_
        << ", misses=" << misses_
        << ", evictions=" << evictions_
        << ", hit_rate=" << std::fixed << hit_rate() * 100 << "%"
        << ")";
    return oss.str();
}

MemoryCache::MemoryCache(size_t max_memory_bytes, CacheStrategy strategy)
    : name_("MemoryCache"), max_memory_bytes_(max_memory_bytes),
      current_memory_bytes_(0), strategy_(strategy) {
}

MemoryCache::~MemoryCache() {
    clear();
}

ResultT<std::optional<std::string>> MemoryCache::get(const std::string& key) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        stats_.record_miss();
        return ResultT<std::optional<std::string>>::ok(std::nullopt);
    }

    if (it->second->is_expired()) {
        entries_.erase(it);
        stats_.record_miss();
        return ResultT<std::optional<std::string>>::ok(std::nullopt);
    }

    it->second->record_access();

    if (strategy_ == CacheStrategy::LRU) {
        auto idx_it = access_order_index_.find(key);
        if (idx_it != access_order_index_.end()) {
            access_order_.erase(idx_it->second);
        }
        access_order_.push_front(key);
        access_order_index_[key] = access_order_.begin();
    }

    stats_.record_hit();
    return ResultT<std::optional<std::string>>::ok(it->second->value());
}

Result MemoryCache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        current_memory_bytes_ -= it->second->size();
        entries_.erase(it);
    }

    auto entry = std::make_shared<CacheEntry>(key, value, ttl_seconds);
    current_memory_bytes_ += entry->size();

    entries_[key] = entry;

    if (strategy_ == CacheStrategy::LRU || strategy_ == CacheStrategy::FIFO) {
        access_order_.push_front(key);
        access_order_index_[key] = access_order_.begin();
    }

    evict_if_needed();

    stats_.record_set();
    return Result::ok();
}

Result MemoryCache::del(const std::string& key) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        current_memory_bytes_ -= it->second->size();
        entries_.erase(it);

        auto idx_it = access_order_index_.find(key);
        if (idx_it != access_order_index_.end()) {
            access_order_.erase(idx_it->second);
            access_order_index_.erase(idx_it);
        }

        stats_.record_delete();
    }

    return Result::ok();
}

ResultT<bool> MemoryCache::exists(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return ResultT<bool>::ok(false);
    }

    if (it->second->is_expired()) {
        return ResultT<bool>::ok(false);
    }

    return ResultT<bool>::ok(true);
}

Result MemoryCache::clear() {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    entries_.clear();
    access_order_.clear();
    access_order_index_.clear();
    current_memory_bytes_ = 0;

    return Result::ok();
}

size_t MemoryCache::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return entries_.size();
}

void MemoryCache::reset_statistics() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    stats_.reset();
}

ResultT<std::vector<std::string>> MemoryCache::keys(const std::string& pattern) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<std::string> result;
    for (const auto& pair : entries_) {
        if (pattern == "*") {
            result.push_back(pair.first);
        } else {
            if (pair.first.find(pattern) != std::string::npos) {
                result.push_back(pair.first);
            }
        }
    }

    return ResultT<std::vector<std::string>>::ok(result);
}

void MemoryCache::cleanup_expired() {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.begin();
    while (it != entries_.end()) {
        if (it->second->is_expired()) {
            current_memory_bytes_ -= it->second->size();
            auto idx_it = access_order_index_.find(it->first);
            if (idx_it != access_order_index_.end()) {
                access_order_.erase(idx_it->second);
                access_order_index_.erase(idx_it);
            }
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t MemoryCache::cleanup_count() const {
    size_t count = 0;
    for (const auto& pair : entries_) {
        if (pair.second->is_expired()) {
            count++;
        }
    }
    return count;
}

void MemoryCache::evict_if_needed() {
    while (current_memory_bytes_ > max_memory_bytes_ && !entries_.empty()) {
        evict_one();
    }
}

void MemoryCache::evict_one() {
    if (entries_.empty()) return;

    std::string key_to_evict;

    switch (strategy_) {
        case CacheStrategy::LRU:
        case CacheStrategy::FIFO: {
            if (!access_order_.empty()) {
                key_to_evict = access_order_.back();
                access_order_.pop_back();
                access_order_index_.erase(key_to_evict);
            }
            break;
        }
        case CacheStrategy::LFU: {
            int min_count = INT_MAX;
            for (const auto& pair : entries_) {
                if (pair.second->access_count() < min_count) {
                    min_count = pair.second->access_count();
                    key_to_evict = pair.first;
                }
            }
            break;
        }
        case CacheStrategy::RANDOM: {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>(entries_.size()) - 1);

            auto it = entries_.begin();
            std::advance(it, dis(gen));
            key_to_evict = it->first;
            break;
        }
        case CacheStrategy::TTL_ONLY: {
            time_t min_expiry = LLONG_MAX;
            for (const auto& pair : entries_) {
                if (pair.second->expires_at() < min_expiry) {
                    min_expiry = pair.second->expires_at();
                    key_to_evict = pair.first;
                }
            }
            break;
        }
    }

    if (!key_to_evict.empty()) {
        auto it = entries_.find(key_to_evict);
        if (it != entries_.end()) {
            current_memory_bytes_ -= it->second->size();
            entries_.erase(it);
            stats_.record_eviction();
        }
    }
}

RedisConfig::RedisConfig()
    : host_("localhost"), port_(6379), database_(0),
      key_prefix_("oms:"), connect_timeout_(1000),
      socket_timeout_(5000), max_connections_(10),
      use_ssl_(false) {
}

RedisCache::RedisCache(const RedisConfig& config)
    : name_("RedisCache"), config_(config), connected_(false) {
}

RedisCache::~RedisCache() {
    disconnect();
}

Result RedisCache::connect() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    connected_ = true;
    return Result::ok();
}

void RedisCache::disconnect() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    connected_ = false;
}

std::string RedisCache::prefixed_key(const std::string& key) const {
    return config_.key_prefix() + key;
}

ResultT<std::optional<std::string>> RedisCache::get(const std::string& key) {
    if (!connected_) {
        return ResultT<std::optional<std::string>>::error(
            ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(full_key);
    if (it == entries_.end()) {
        stats_.record_miss();
        return ResultT<std::optional<std::string>>::ok(std::nullopt);
    }

    if (it->second->is_expired()) {
        entries_.erase(it);
        stats_.record_miss();
        return ResultT<std::optional<std::string>>::ok(std::nullopt);
    }

    it->second->record_access();
    stats_.record_hit();
    return ResultT<std::optional<std::string>>::ok(it->second->value());
}

Result RedisCache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    if (!connected_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto entry = std::make_shared<CacheEntry>(full_key, value, ttl_seconds);
    entries_[full_key] = entry;

    stats_.record_set();
    return Result::ok();
}

Result RedisCache::del(const std::string& key) {
    if (!connected_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::lock_guard<std::shared_mutex> lock(mutex_);

    entries_.erase(full_key);
    stats_.record_delete();

    return Result::ok();
}

ResultT<bool> RedisCache::exists(const std::string& key) {
    if (!connected_) {
        return ResultT<bool>::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(full_key);
    if (it == entries_.end()) {
        return ResultT<bool>::ok(false);
    }

    if (it->second->is_expired()) {
        return ResultT<bool>::ok(false);
    }

    return ResultT<bool>::ok(true);
}

Result RedisCache::clear() {
    if (!connected_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    entries_.clear();

    return Result::ok();
}

size_t RedisCache::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return entries_.size();
}

void RedisCache::reset_statistics() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    stats_.reset();
}

ResultT<std::vector<std::string>> RedisCache::keys(const std::string& pattern) {
    if (!connected_) {
        return ResultT<std::vector<std::string>>::error(
            ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<std::string> result;
    std::string full_pattern = prefixed_key(pattern);

    for (const auto& pair : entries_) {
        if (pattern == "*" || pair.first.find(pattern) != std::string::npos) {
            result.push_back(pair.first);
        }
    }

    return ResultT<std::vector<std::string>>::ok(result);
}

Result RedisCache::expire(const std::string& key, int seconds) {
    if (!connected_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(full_key);
    if (it != entries_.end()) {
        it->second->set_ttl(seconds);
    }

    return Result::ok();
}

ResultT<int> RedisCache::ttl(const std::string& key) {
    if (!connected_) {
        return ResultT<int>::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(full_key);
    if (it == entries_.end()) {
        return ResultT<int>::ok(-2);
    }

    return ResultT<int>::ok(it->second->ttl());
}

ResultT<size_t> RedisCache::increment(const std::string& key, int amount) {
    (void)key;
    (void)amount;
    return ResultT<size_t>::ok(0);
}

ResultT<size_t> RedisCache::decrement(const std::string& key, int amount) {
    (void)key;
    (void)amount;
    return ResultT<size_t>::ok(0);
}

ResultT<bool> RedisCache::set_nx(const std::string& key, const std::string& value, int ttl_seconds) {
    if (!connected_) {
        return ResultT<bool>::error(ErrorCode::CACHE_ERROR, "Redis not connected");
    }

    std::string full_key = prefixed_key(key);

    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = entries_.find(full_key);
    if (it != entries_.end()) {
        return ResultT<bool>::ok(false);
    }

    auto entry = std::make_shared<CacheEntry>(full_key, value, ttl_seconds);
    entries_[full_key] = entry;

    stats_.record_set();
    return ResultT<bool>::ok(true);
}

MultiTierCache::MultiTierCache()
    : name_("MultiTierCache"), write_through_(true), read_through_(true) {
}

MultiTierCache::~MultiTierCache() {
}

void MultiTierCache::add_backend(std::shared_ptr<CacheBackend> backend) {
    backends_.push_back(backend);
}

ResultT<std::optional<std::string>> MultiTierCache::get(const std::string& key) {
    for (size_t i = 0; i < backends_.size(); i++) {
        auto result = backends_[i]->get(key);
        if (!result) {
            continue;
        }
        if (result.value()) {
            if (read_through_ && i > 0) {
                for (size_t j = 0; j < i; j++) {
                    backends_[j]->set(key, result.value().value(), 3600);
                }
            }
            stats_.record_hit();
            return result;
        }
    }

    stats_.record_miss();
    return ResultT<std::optional<std::string>>::ok(std::nullopt);
}

Result MultiTierCache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    Result last_result = Result::ok();

    for (auto& backend : backends_) {
        last_result = backend->set(key, value, ttl_seconds);
        if (!write_through_ && !last_result) {
            break;
        }
    }

    stats_.record_set();
    return last_result;
}

Result MultiTierCache::del(const std::string& key) {
    for (auto& backend : backends_) {
        backend->del(key);
    }

    stats_.record_delete();
    return Result::ok();
}

ResultT<bool> MultiTierCache::exists(const std::string& key) {
    for (auto& backend : backends_) {
        auto result = backend->exists(key);
        if (result && result.value()) {
            return ResultT<bool>::ok(true);
        }
    }
    return ResultT<bool>::ok(false);
}

Result MultiTierCache::clear() {
    for (auto& backend : backends_) {
        backend->clear();
    }
    return Result::ok();
}

size_t MultiTierCache::size() const {
    size_t total = 0;
    for (const auto& backend : backends_) {
        total += backend->size();
    }
    return total;
}

size_t MultiTierCache::max_size() const {
    size_t total = 0;
    for (const auto& backend : backends_) {
        total += backend->max_size();
    }
    return total;
}

void MultiTierCache::reset_statistics() {
    stats_.reset();
    for (auto& backend : backends_) {
        backend->reset_statistics();
    }
}

bool MultiTierCache::is_available() const {
    for (const auto& backend : backends_) {
        if (backend->is_available()) {
            return true;
        }
    }
    return false;
}

ResultT<std::vector<std::string>> MultiTierCache::keys(const std::string& pattern) {
    if (backends_.empty()) {
        return ResultT<std::vector<std::string>>::ok(std::vector<std::string>());
    }
    return backends_[0]->keys(pattern);
}

CacheManager& CacheManager::instance() {
    static CacheManager instance;
    return instance;
}

CacheManager::CacheManager()
    : tier_(CacheTier::MEMORY) {
}

CacheManager::~CacheManager() {
}

Result CacheManager::init(CacheTier tier, size_t memory_limit, const RedisConfig* redis_config) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    tier_ = tier;

    if (tier == CacheTier::MEMORY || tier == CacheTier::MEMORY_REDIS) {
        auto memory_cache = std::make_shared<MemoryCache>(memory_limit);
        backend_ = memory_cache;

        if (tier == CacheTier::MEMORY_REDIS && redis_config) {
            auto multi_tier = std::make_shared<MultiTierCache>();
            multi_tier->add_backend(memory_cache);

            auto redis_cache = std::make_shared<RedisCache>(*redis_config);
            redis_cache->connect();
            multi_tier->add_backend(redis_cache);

            backend_ = multi_tier;
        }
    } else if (tier == CacheTier::REDIS && redis_config) {
        auto redis_cache = std::make_shared<RedisCache>(*redis_config);
        redis_cache->connect();
        backend_ = redis_cache;
    }

    return Result::ok();
}

void CacheManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    backend_.reset();
}

ResultT<std::optional<std::string>> CacheManager::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!backend_) {
        return ResultT<std::optional<std::string>>::error(
            ErrorCode::CACHE_ERROR, "Cache not initialized");
    }
    return backend_->get(key);
}

Result CacheManager::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!backend_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Cache not initialized");
    }
    return backend_->set(key, value, ttl_seconds);
}

Result CacheManager::del(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!backend_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Cache not initialized");
    }
    return backend_->del(key);
}

ResultT<bool> CacheManager::exists(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!backend_) {
        return ResultT<bool>::error(ErrorCode::CACHE_ERROR, "Cache not initialized");
    }
    return backend_->exists(key);
}

Result CacheManager::clear() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!backend_) {
        return Result::error(ErrorCode::CACHE_ERROR, "Cache not initialized");
    }
    return backend_->clear();
}

const CacheStatistics& CacheManager::statistics() const {
    static CacheStatistics empty;
    if (!backend_) {
        return empty;
    }
    return backend_->statistics();
}

void CacheManager::reset_statistics() {
    if (backend_) {
        backend_->reset_statistics();
    }
}

bool CacheManager::is_available() const {
    return backend_ && backend_->is_available();
}

size_t CacheManager::size() const {
    if (!backend_) {
        return 0;
    }
    return backend_->size();
}

ResultT<std::vector<std::string>> CacheManager::keys(const std::string& pattern) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!backend_) {
        return ResultT<std::vector<std::string>>::error(
            ErrorCode::CACHE_ERROR, "Cache not initialized");
    }
    return backend_->keys(pattern);
}

void CacheManager::cleanup() {
    if (auto* memory_cache = dynamic_cast<MemoryCache*>(backend_.get())) {
        memory_cache->cleanup_expired();
    }
}

CacheLock::CacheLock(CacheBackend& backend, const std::string& key, int timeout_ms)
    : backend_(backend), key_("lock:" + key), acquired_(false) {
    try_acquire(timeout_ms);
}

CacheLock::~CacheLock() {
    if (acquired_) {
        release();
    }
}

bool CacheLock::try_acquire(int timeout_ms) {
    (void)timeout_ms;
    if (auto* redis = dynamic_cast<RedisCache*>(&backend_)) {
        auto result = redis->set_nx(key_, "1", 10);
        acquired_ = result && result.value();
    } else {
        acquired_ = true;
    }
    return acquired_;
}

void CacheLock::release() {
    backend_.del(key_);
    acquired_ = false;
}

} // namespace cache
} // namespace oms
