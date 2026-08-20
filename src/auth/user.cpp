#include "auth/user.h"
#include "auth/rbac.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace oms {
namespace auth {

User::User()
    : id_(0), role_(UserRole::GUEST), is_active_(false) {
}

User::User(UserId id, const std::string& username)
    : id_(id), username_(username), role_(UserRole::CUSTOMER), is_active_(true) {
}

UserId User::id() const { return id_; }
const std::string& User::username() const { return username_; }
UserRole User::role() const { return role_; }
void User::set_role(UserRole role) { role_ = role; }

const UserProfile& User::profile() const { return profile_; }
UserProfile& User::profile() { return profile_; }
void User::set_profile(const UserProfile& p) { profile_ = p; }

const UserSecurity& User::security() const { return security_; }
UserSecurity& User::security() { return security_; }

const UserStats& User::stats() const { return stats_; }
UserStats& User::stats() { return stats_; }

bool User::is_active() const { return is_active_; }
void User::set_active(bool active) { is_active_ = active; }

bool User::is_vip() const {
    return static_cast<int>(role_) >= static_cast<int>(UserRole::VIP_CUSTOMER);
}

bool User::is_admin() const {
    return static_cast<int>(role_) >= static_cast<int>(UserRole::ADMIN);
}

bool User::has_permission(PermissionId permission) const {
    return permissions_.count(permission) > 0;
}

void User::grant_permission(PermissionId permission) {
    permissions_.insert(permission);
}

void User::revoke_permission(PermissionId permission) {
    permissions_.erase(permission);
}

const std::set<PermissionId>& User::permissions() const {
    return permissions_;
}

Result User::check_password(const std::string& password) const {
    std::string hashed = hash_password(password, security_.password_salt);
    if (hashed == security_.password_hash) {
        return Result::ok();
    }
    return Result::error(ErrorCode::AUTH_FAILED, "Invalid password");
}

Result User::change_password(const std::string& old_password, const std::string& new_password) {
    Result check = check_password(old_password);
    if (!check) {
        return check;
    }

    std::string new_salt = generate_salt();
    security_.password_hash = hash_password(new_password, new_salt);
    security_.password_salt = new_salt;
    security_.password_changed_at = time(nullptr);

    return Result::ok();
}

Result User::lock_account(int duration_sec) {
    security_.is_locked = true;
    security_.locked_until = time(nullptr) + duration_sec;
    return Result::ok();
}

Result User::unlock_account() {
    security_.is_locked = false;
    security_.locked_until = 0;
    return Result::ok();
}

bool User::is_locked() const {
    if (!security_.is_locked) {
        return false;
    }
    return time(nullptr) < security_.locked_until;
}

void User::record_login(const std::string& ip) {
    (void)ip;
    stats_.last_login_at = time(nullptr);
    stats_.total_logins++;
}

std::string User::to_string() const {
    std::ostringstream oss;
    oss << "User(id=" << id_
        << ", username=" << username_
        << ", role=" << static_cast<int>(role_)
        << ", active=" << (is_active_ ? "true" : "false")
        << ")";
    return oss.str();
}

std::string User::hash_password(const std::string& password, const std::string& salt) {
    std::ostringstream oss;
    oss << salt << ":" << password;
    std::string combined = oss.str();

    uint64_t hash = 0;
    for (char c : combined) {
        hash = hash * 31 + static_cast<uint8_t>(c);
    }

    std::ostringstream hex_oss;
    hex_oss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return hex_oss.str();
}

std::string User::generate_salt() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::ostringstream oss;
    for (int i = 0; i < 16; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return oss.str();
}

UserManager& UserManager::instance() {
    static UserManager instance;
    return instance;
}

UserManager::UserManager() {
}

UserManager::~UserManager() {
}

Result UserManager::init() {
    return Result::ok();
}

void UserManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    users_.clear();
    username_index_.clear();
}

ResultT<User*> UserManager::create_user(const std::string& username,
                                          const std::string& password,
                                          UserRole role) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    for (const auto& pair : users_) {
        if (pair.second->username() == username) {
            return ResultT<User*>::error(ErrorCode::USER_ALREADY_EXISTS, "Username already exists");
        }
    }

    UserId id = users_.size() + 1;
    auto user = std::make_unique<User>(id, username);
    user->set_role(role);

    std::string salt = User::generate_salt();
    user->security().password_hash = User::hash_password(password, salt);
    user->security().password_salt = salt;

    auto user_ptr = user.get();
    users_[id] = std::move(user);
    username_index_[username] = id;

    return ResultT<User*>::ok(user_ptr);
}

Result UserManager::delete_user(UserId id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = users_.find(id);
    if (it == users_.end()) {
        return Result::error(ErrorCode::USER_NOT_FOUND, "User not found");
    }

    username_index_.erase(it->second->username());
    users_.erase(it);
    return Result::ok();
}

ResultT<User*> UserManager::get_user(UserId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = users_.find(id);
    if (it == users_.end()) {
        return ResultT<User*>::error(ErrorCode::USER_NOT_FOUND, "User not found");
    }

    return ResultT<User*>::ok(it->second.get());
}

ResultT<User*> UserManager::get_user_by_username(const std::string& username) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = username_index_.find(username);
    if (it == username_index_.end()) {
        return ResultT<User*>::error(ErrorCode::USER_NOT_FOUND, "User not found");
    }

    return get_user(it->second);
}

Result UserManager::update_user_profile(UserId id, const UserProfile& profile) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->set_profile(profile);
    return Result::ok();
}

Result UserManager::update_user_role(UserId id, UserRole new_role) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->set_role(new_role);
    return Result::ok();
}

Result UserManager::activate_user(UserId id) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->set_active(true);
    return Result::ok();
}

Result UserManager::deactivate_user(UserId id) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->set_active(false);
    return Result::ok();
}

ResultT<std::vector<User*>> UserManager::list_users(size_t page, size_t page_size) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<User*> result;
    size_t start = page * page_size;
    size_t end = start + page_size;
    size_t index = 0;

    for (const auto& pair : users_) {
        if (index >= start && index < end) {
            result.push_back(pair.second.get());
        }
        index++;
        if (index >= end) {
            break;
        }
    }

    return ResultT<std::vector<User*>>::ok(result);
}

ResultT<size_t> UserManager::get_user_count() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return ResultT<size_t>::ok(users_.size());
}

ResultT<std::vector<User*>> UserManager::search_users(const std::string& keyword,
                                                         size_t page, size_t page_size) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<User*> all_matches;
    for (const auto& pair : users_) {
        if (pair.second->username().find(keyword) != std::string::npos ||
            pair.second->profile().email.find(keyword) != std::string::npos) {
            all_matches.push_back(pair.second.get());
        }
    }

    std::vector<User*> result;
    size_t start = page * page_size;
    size_t end = std::min(start + page_size, all_matches.size());

    for (size_t i = start; i < end; i++) {
        result.push_back(all_matches[i]);
    }

    return ResultT<std::vector<User*>>::ok(result);
}

Result UserManager::grant_permission(UserId id, PermissionId permission) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->grant_permission(permission);
    return Result::ok();
}

Result UserManager::revoke_permission(UserId id, PermissionId permission) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->revoke_permission(permission);
    return Result::ok();
}

Result UserManager::reset_password(UserId id, const std::string& new_password) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    User* user = result.value();
    std::string new_salt = User::generate_salt();
    user->security().password_hash = User::hash_password(new_password, new_salt);
    user->security().password_salt = new_salt;

    return Result::ok();
}

Result UserManager::force_password_reset(UserId id) {
    auto result = get_user(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }
    result.value()->security().require_password_change = true;
    return Result::ok();
}

bool UserManager::validate_password_strength(const std::string& password) {
    if (password.length() < 8) return false;

    bool has_upper = false, has_lower = false, has_digit = false, has_special = false;
    for (size_t i = 0; i < password.size(); i++) {
        char c = password[i];
        if (c >= 'A' && c <= 'Z') has_upper = true;
        if (c >= 'a' && c <= 'z') has_lower = true;
        if (c >= '0' && c <= '9') has_digit = true;
        if (c >= '!' && c <= '/') has_special = true;
    }

    int score = has_upper + has_lower + has_digit + has_special;
    return score >= 3;
}

std::string UserManager::generate_random_password(size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* buffer = new char[length];

    for (size_t i = 0; i < length; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }

    std::string result(buffer);
    return result;
}

Result UserManager::rate_limit_login(const std::string& ip_address) {
    time_t now = time(nullptr);
    auto it = lockout_expiry_.find(ip_address);
    if (it != lockout_expiry_.end() && now < it->second) {
        return Result::error(ErrorCode::AUTH_FAILED, "Account locked");
    }

    int failures = login_failure_count_[ip_address];
    if (failures >= 5) {
        lockout_expiry_[ip_address] = now + 300;
        return Result::error(ErrorCode::AUTH_FAILED, "Too many attempts");
    }

    return Result::ok();
}

ResultT<UserId> UserManager::batch_create_users(const std::vector<std::string>& usernames,
                                                  const std::vector<std::string>& passwords,
                                                  UserRole default_role) {
    UserId last_id = 0;

    for (size_t i = 0; i < usernames.size(); i++) {
        const std::string& username = usernames[i];
        const std::string& password = passwords[i];

        if (!validate_password_strength(password)) {
            continue;
        }

        auto result = create_user(username, password, default_role);
        if (result) {
            last_id = result.value()->id();
        }
    }

    return ResultT<UserId>::ok(last_id);
}

Result UserManager::batch_delete_users(const std::vector<UserId>& user_ids) {
    std::vector<Result> results;

    for (size_t i = 0; i < user_ids.size(); i++) {
        delete_user(user_ids[i]);
    }

    return Result::ok();
}

Result UserManager::batch_update_role(const std::vector<UserId>& user_ids, UserRole new_role) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    for (UserId id : user_ids) {
        auto it = users_.find(id);
        if (it != users_.end()) {
            it->second->set_role(new_role);
        }
    }

    return Result::ok();
}

} // namespace auth
} // namespace oms
