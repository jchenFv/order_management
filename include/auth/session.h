#ifndef OMS_AUTH_SESSION_H
#define OMS_AUTH_SESSION_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <chrono>
#include <map>
#include <thread>

namespace oms {
namespace auth {

struct SessionData {
    UserId user_id;
    std::string username;
    UserRole role;
    std::string ip_address;
    std::string user_agent;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_activity;
    bool is_persistent;
    std::map<std::string, std::string> custom_data;
};

class Session {
public:
    Session();
    Session(const SessionId& id, UserId user_id, UserRole role);

    const SessionId& id() const;
    UserId user_id() const;
    UserRole role() const;

    const std::string& ip_address() const;
    void set_ip_address(const std::string& ip);

    const std::string& user_agent() const;
    void set_user_agent(const std::string& ua);

    std::chrono::system_clock::time_point created_at() const;
    std::chrono::system_clock::time_point expires_at() const;
    std::chrono::system_clock::time_point last_activity() const;

    void update_activity();
    void extend_expiration(int seconds);
    bool is_expired() const;
    bool is_valid() const;

    bool is_persistent() const;
    void set_persistent(bool persistent);

    Result set_custom_data(const std::string& key, const std::string& value);
    ResultT<std::string> get_custom_data(const std::string& key) const;
    bool has_custom_data(const std::string& key) const;
    void clear_custom_data(const std::string& key);

    const SessionData& data() const;

private:
    SessionId id_;
    SessionData data_;
};

class SessionManager {
public:
    static SessionManager& instance();

    Result init();
    void shutdown();

    ResultT<SessionId> create_session(UserId user_id, UserRole role,
                                       const std::string& ip = "",
                                       const std::string& user_agent = "",
                                       bool persistent = false);

    Result destroy_session(const SessionId& id);
    Result destroy_all_user_sessions(UserId user_id);

    ResultT<Session*> get_session(const SessionId& id);
    ResultT<Session*> get_session_no_lock(const SessionId& id);

    Result refresh_session(const SessionId& id);

    bool session_exists(const SessionId& id) const;
    bool is_session_valid(const SessionId& id) const;

    ResultT<std::vector<SessionId>> get_user_sessions(UserId user_id);
    ResultT<size_t> get_active_session_count() const;
    ResultT<size_t> get_user_session_count(UserId user_id) const;

    Result cleanup_expired_sessions();
    void start_cleanup_thread();
    void stop_cleanup_thread();

    Result set_session_data(const SessionId& id, const std::string& key, const std::string& value);
    ResultT<std::string> get_session_data(const SessionId& id, const std::string& key);

    int default_session_duration() const;
    void set_default_session_duration(int seconds);

private:
    SessionManager();
    ~SessionManager();

    static SessionId generate_session_id();

    std::map<SessionId, std::unique_ptr<Session>> sessions_;
    std::map<UserId, std::vector<SessionId>> user_session_index_;
    mutable std::shared_mutex mutex_;

    int default_session_duration_;

    std::atomic<bool> cleanup_running_;
    std::thread cleanup_thread_;
};

} // namespace auth
} // namespace oms

#endif // OMS_AUTH_SESSION_H
