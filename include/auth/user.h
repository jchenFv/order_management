#ifndef OMS_AUTH_USER_H
#define OMS_AUTH_USER_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <set>

namespace oms {
namespace auth {

struct UserProfile {
    std::string nickname;
    std::string avatar_url;
    std::string email;
    std::string phone;
    std::string language;
    std::string timezone;
    std::map<std::string, std::string> metadata;
};

struct UserSecurity {
    std::string password_hash;
    std::string password_salt;
    int failed_login_attempts;
    time_t last_failed_login;
    time_t password_changed_at;
    bool is_locked;
    time_t locked_until;
    bool require_password_change;
    std::vector<std::string> two_fa_backup_codes;
    std::string two_fa_secret;
    bool two_fa_enabled;
};

struct UserStats {
    time_t created_at;
    time_t updated_at;
    time_t last_login_at;
    std::string last_login_ip;
    int total_logins;
    int total_orders;
    double total_spent;
    int vip_points;
    int vip_level;
};

class User {
public:
    User();
    User(UserId id, const std::string& username);

    UserId id() const;
    const std::string& username() const;
    UserRole role() const;
    void set_role(UserRole role);

    const UserProfile& profile() const;
    UserProfile& profile();
    void set_profile(const UserProfile& profile);

    const UserSecurity& security() const;
    UserSecurity& security();

    const UserStats& stats() const;
    UserStats& stats();

    bool is_active() const;
    void set_active(bool active);

    bool is_vip() const;
    bool is_admin() const;
    bool has_permission(PermissionId permission) const;

    void grant_permission(PermissionId permission);
    void revoke_permission(PermissionId permission);
    const std::set<PermissionId>& permissions() const;

    Result check_password(const std::string& password) const;
    Result change_password(const std::string& old_password, const std::string& new_password);

    Result lock_account(int duration_sec);
    Result unlock_account();
    bool is_locked() const;

    void record_login(const std::string& ip);

    std::string to_string() const;

    static std::string hash_password(const std::string& password, const std::string& salt);
    static std::string generate_salt();

private:
    UserId id_;
    std::string username_;
    UserRole role_;
    bool is_active_;
    UserProfile profile_;
    UserSecurity security_;
    UserStats stats_;
    std::set<PermissionId> permissions_;
};

class UserManager {
public:
    static UserManager& instance();

    Result init();
    void shutdown();

    ResultT<User*> create_user(const std::string& username, const std::string& password,
                                 UserRole role = UserRole::CUSTOMER);
    Result delete_user(UserId id);
    ResultT<User*> get_user(UserId id);
    ResultT<User*> get_user_by_username(const std::string& username);

    Result update_user_profile(UserId id, const UserProfile& profile);
    Result update_user_role(UserId id, UserRole new_role);
    Result activate_user(UserId id);
    Result deactivate_user(UserId id);

    ResultT<std::vector<User*>> list_users(size_t page = 0, size_t page_size = 50);
    ResultT<size_t> get_user_count();

    ResultT<std::vector<User*>> search_users(const std::string& keyword,
                                               size_t page = 0, size_t page_size = 50);

    Result grant_permission(UserId id, PermissionId permission);
    Result revoke_permission(UserId id, PermissionId permission);

    Result reset_password(UserId id, const std::string& new_password);
    Result force_password_reset(UserId id);

    ResultT<UserId> batch_create_users(const std::vector<std::string>& usernames,
                                         const std::vector<std::string>& passwords,
                                         UserRole default_role);
    Result batch_delete_users(const std::vector<UserId>& user_ids);
    Result batch_update_role(const std::vector<UserId>& user_ids, UserRole new_role);

    static bool validate_password_strength(const std::string& password);
    static std::string generate_random_password(size_t length);

    Result rate_limit_login(const std::string& ip_address);

private:
    UserManager();
    ~UserManager();

    std::map<UserId, std::unique_ptr<User>> users_;
    std::map<std::string, int> login_failure_count_;
    std::map<std::string, time_t> lockout_expiry_;
    std::map<std::string, UserId> username_index_;
    mutable std::shared_mutex mutex_;
};

} // namespace auth
} // namespace oms

#endif // OMS_AUTH_USER_H
