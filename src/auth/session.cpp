#include "auth/session.h"
#include <algorithm>
#include <random>

namespace oms {
namespace auth {

Session::Session()
    : id_("") {
    data_.user_id = 0;
    data_.role = UserRole::GUEST;
    data_.created_at = std::chrono::system_clock::now();
    data_.expires_at = std::chrono::system_clock::now() + std::chrono::hours(24);
    data_.last_activity = std::chrono::system_clock::now();
    data_.is_persistent = false;
}

Session::Session(const SessionId& id, UserId user_id, UserRole role)
    : id_(id) {
    data_.user_id = user_id;
    data_.role = role;
    data_.created_at = std::chrono::system_clock::now();
    data_.expires_at = std::chrono::system_clock::now() + std::chrono::hours(24);
    data_.last_activity = std::chrono::system_clock::now();
    data_.is_persistent = false;
}

const SessionId& Session::id() const {
    return id_;
}

UserId Session::user_id() const {
    return data_.user_id;
}

UserRole Session::role() const {
    return data_.role;
}

const std::string& Session::ip_address() const {
    return data_.ip_address;
}

void Session::set_ip_address(const std::string& ip) {
    data_.ip_address = ip;
}

const std::string& Session::user_agent() const {
    return data_.user_agent;
}

void Session::set_user_agent(const std::string& ua) {
    data_.user_agent = ua;
}

std::chrono::system_clock::time_point Session::created_at() const {
    return data_.created_at;
}

std::chrono::system_clock::time_point Session::expires_at() const {
    return data_.expires_at;
}

std::chrono::system_clock::time_point Session::last_activity() const {
    return data_.last_activity;
}

void Session::update_activity() {
    data_.last_activity = std::chrono::system_clock::now();
}

void Session::extend_expiration(int seconds) {
    data_.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(seconds);
}

bool Session::is_expired() const {
    return std::chrono::system_clock::now() > data_.expires_at;
}

bool Session::is_valid() const {
    return !is_expired() && data_.user_id != 0;
}

bool Session::is_persistent() const {
    return data_.is_persistent;
}

void Session::set_persistent(bool persistent) {
    data_.is_persistent = persistent;
}

Result Session::set_custom_data(const std::string& key, const std::string& value) {
    data_.custom_data[key] = value;
    return Result::ok();
}

ResultT<std::string> Session::get_custom_data(const std::string& key) const {
    auto it = data_.custom_data.find(key);
    if (it == data_.custom_data.end()) {
        return ResultT<std::string>::error(ErrorCode::INVALID_PARAMETER, "Key not found");
    }
    return ResultT<std::string>::ok(it->second);
}

bool Session::has_custom_data(const std::string& key) const {
    return data_.custom_data.count(key) > 0;
}

void Session::clear_custom_data(const std::string& key) {
    data_.custom_data.erase(key);
}

const SessionData& Session::data() const {
    return data_;
}

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager()
    : default_session_duration_(86400), cleanup_running_(false) {
}

SessionManager::~SessionManager() {
    shutdown();
}

Result SessionManager::init() {
    start_cleanup_thread();
    return Result::ok();
}

void SessionManager::shutdown() {
    stop_cleanup_thread();
    std::lock_guard<std::shared_mutex> lock(mutex_);
    sessions_.clear();
    user_session_index_.clear();
}

SessionId SessionManager::generate_session_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    return "sess_" + std::to_string(dis(gen)) + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

ResultT<SessionId> SessionManager::create_session(UserId user_id, UserRole role,
                                                   const std::string& ip,
                                                   const std::string& user_agent,
                                                   bool persistent) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    SessionId id = generate_session_id();
    auto session = std::make_unique<Session>(id, user_id, role);
    session->set_ip_address(ip);
    session->set_user_agent(user_agent);
    session->set_persistent(persistent);

    if (!persistent) {
        session->extend_expiration(default_session_duration_);
    }

    sessions_[id] = std::move(session);
    user_session_index_[user_id].push_back(id);

    return ResultT<SessionId>::ok(id);
}

Result SessionManager::destroy_session(const SessionId& id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        return Result::error(ErrorCode::AUTH_REQUIRED, "Session not found");
    }

    UserId user_id = it->second->user_id();
    sessions_.erase(it);

    auto& user_sessions = user_session_index_[user_id];
    user_sessions.erase(std::remove(user_sessions.begin(), user_sessions.end(), id), user_sessions.end());

    return Result::ok();
}

Result SessionManager::destroy_all_user_sessions(UserId user_id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = user_session_index_.find(user_id);
    if (it == user_session_index_.end()) {
        return Result::ok();
    }

    for (const auto& session_id : it->second) {
        sessions_.erase(session_id);
    }
    user_session_index_.erase(it);

    return Result::ok();
}

ResultT<Session*> SessionManager::get_session(const SessionId& id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return get_session_no_lock(id);
}

ResultT<Session*> SessionManager::get_session_no_lock(const SessionId& id) {
    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        return ResultT<Session*>::error(ErrorCode::AUTH_REQUIRED, "Session not found");
    }

    if (it->second->is_expired()) {
        return ResultT<Session*>::error(ErrorCode::AUTH_REQUIRED, "Session expired");
    }

    it->second->update_activity();
    return ResultT<Session*>::ok(it->second.get());
}

Result SessionManager::refresh_session(const SessionId& id) {
    auto result = get_session(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->extend_expiration(default_session_duration_);
    return Result::ok();
}

bool SessionManager::session_exists(const SessionId& id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return sessions_.count(id) > 0;
}

bool SessionManager::is_session_valid(const SessionId& id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        return false;
    }

    return !it->second->is_expired();
}

ResultT<std::vector<SessionId>> SessionManager::get_user_sessions(UserId user_id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<SessionId> result;
    auto it = user_session_index_.find(user_id);
    if (it != user_session_index_.end()) {
        result = it->second;
    }

    return ResultT<std::vector<SessionId>>::ok(result);
}

ResultT<size_t> SessionManager::get_active_session_count() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return ResultT<size_t>::ok(sessions_.size());
}

ResultT<size_t> SessionManager::get_user_session_count(UserId user_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = user_session_index_.find(user_id);
    if (it == user_session_index_.end()) {
        return ResultT<size_t>::ok(0);
    }
    return ResultT<size_t>::ok(it->second.size());
}

Result SessionManager::cleanup_expired_sessions() {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    std::vector<SessionId> expired_ids;
    for (const auto& pair : sessions_) {
        if (pair.second->is_expired()) {
            expired_ids.push_back(pair.first);
        }
    }

    for (const auto& id : expired_ids) {
        UserId user_id = sessions_[id]->user_id();
        sessions_.erase(id);

        auto& user_sessions = user_session_index_[user_id];
        user_sessions.erase(std::remove(user_sessions.begin(), user_sessions.end(), id), user_sessions.end());
    }

    return Result::ok();
}

void SessionManager::start_cleanup_thread() {
    cleanup_running_ = true;
    cleanup_thread_ = std::thread([this]() {
        while (cleanup_running_) {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            if (cleanup_running_) {
                cleanup_expired_sessions();
            }
        }
    });
}

void SessionManager::stop_cleanup_thread() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

Result SessionManager::set_session_data(const SessionId& id, const std::string& key, const std::string& value) {
    auto result = get_session(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    return result.value()->set_custom_data(key, value);
}

ResultT<std::string> SessionManager::get_session_data(const SessionId& id, const std::string& key) {
    auto result = get_session(id);
    if (!result) {
        return ResultT<std::string>::error(result.error_code(), result.error_message());
    }
    return result.value()->get_custom_data(key);
}

int SessionManager::default_session_duration() const {
    return default_session_duration_;
}

void SessionManager::set_default_session_duration(int seconds) {
    default_session_duration_ = seconds;
}

} // namespace auth
} // namespace oms
