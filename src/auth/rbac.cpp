#include "auth/rbac.h"
#include <algorithm>

namespace oms {
namespace auth {

bool Role::has_permission(PermissionId id) const {
    return permissions.count(id) > 0;
}

bool Role::inherits_from(RoleId role_id) const {
    return inherited_roles.count(role_id) > 0;
}

PermissionManager& PermissionManager::instance() {
    static PermissionManager instance;
    return instance;
}

PermissionManager::PermissionManager() {
}

PermissionManager::~PermissionManager() {
}

Result PermissionManager::init() {
    return Result::ok();
}

void PermissionManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    permissions_.clear();
    name_index_.clear();
}

ResultT<PermissionId> PermissionManager::register_permission(const std::string& name,
                                                             const std::string& description,
                                                             const std::string& resource,
                                                             const std::string& action,
                                                             bool is_system) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    PermissionId id = permissions_.size() + 1;
    auto permission = std::make_unique<Permission>();
    permission->id = id;
    permission->name = name;
    permission->description = description;
    permission->resource = resource;
    permission->action = action;
    permission->is_system = is_system;
    permission->is_enabled = true;

    permissions_[id] = std::move(permission);
    name_index_[name] = id;

    return ResultT<PermissionId>::ok(id);
}

Result PermissionManager::unregister_permission(PermissionId id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = permissions_.find(id);
    if (it == permissions_.end()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Permission not found");
    }

    name_index_.erase(it->second->name);
    permissions_.erase(it);

    return Result::ok();
}

ResultT<Permission*> PermissionManager::get_permission(PermissionId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = permissions_.find(id);
    if (it == permissions_.end()) {
        return ResultT<Permission*>::error(ErrorCode::INVALID_PARAMETER, "Permission not found");
    }

    return ResultT<Permission*>::ok(it->second.get());
}

ResultT<Permission*> PermissionManager::get_permission_by_name(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = name_index_.find(name);
    if (it == name_index_.end()) {
        return ResultT<Permission*>::error(ErrorCode::INVALID_PARAMETER, "Permission not found");
    }

    return get_permission(it->second);
}

ResultT<std::vector<Permission*>> PermissionManager::list_permissions() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Permission*> result;
    for (const auto& pair : permissions_) {
        result.push_back(pair.second.get());
    }

    return ResultT<std::vector<Permission*>>::ok(result);
}

ResultT<std::vector<Permission*>> PermissionManager::get_permissions_by_resource(const std::string& resource) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Permission*> result;
    for (const auto& pair : permissions_) {
        if (pair.second->resource == resource) {
            result.push_back(pair.second.get());
        }
    }

    return ResultT<std::vector<Permission*>>::ok(result);
}

Result PermissionManager::enable_permission(PermissionId id) {
    auto result = get_permission(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    result.value()->is_enabled = true;
    return Result::ok();
}

Result PermissionManager::disable_permission(PermissionId id) {
    auto result = get_permission(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    result.value()->is_enabled = false;
    return Result::ok();
}

bool PermissionManager::has_permission(UserRole role, PermissionId permission) const {
    (void)role;
    (void)permission;
    return true;
}

bool PermissionManager::has_permission(const std::set<PermissionId>& user_perms,
                                      PermissionId permission) const {
    return user_perms.count(permission) > 0;
}

RoleManager& RoleManager::instance() {
    static RoleManager instance;
    return instance;
}

RoleManager::RoleManager()
    : cache_dirty_(false) {
}

RoleManager::~RoleManager() {
}

Result RoleManager::init() {
    return Result::ok();
}

void RoleManager::shutdown() {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    roles_.clear();
    inheritance_cache_.clear();
}

ResultT<RoleId> RoleManager::create_role(const std::string& name,
                                           const std::string& description,
                                           UserRole level) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    RoleId id = roles_.size() + 1;
    auto role = std::make_unique<Role>();
    role->id = id;
    role->name = name;
    role->description = description;
    role->level = level;
    role->is_system = false;
    role->is_enabled = true;

    roles_[id] = std::move(role);

    return ResultT<RoleId>::ok(id);
}

Result RoleManager::delete_role(RoleId id) {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    auto it = roles_.find(id);
    if (it == roles_.end()) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Role not found");
    }

    if (it->second->is_system) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Cannot delete system role");
    }

    roles_.erase(it);
    invalidate_inheritance_cache();

    return Result::ok();
}

ResultT<Role*> RoleManager::get_role(RoleId id) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = roles_.find(id);
    if (it == roles_.end()) {
        return ResultT<Role*>::error(ErrorCode::INVALID_PARAMETER, "Role not found");
    }

    return ResultT<Role*>::ok(it->second.get());
}

Result RoleManager::add_permission_to_role(RoleId role_id, PermissionId permission_id) {
    auto result = get_role(role_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->permissions.insert(permission_id);
    invalidate_inheritance_cache();

    return Result::ok();
}

Result RoleManager::remove_permission_from_role(RoleId role_id, PermissionId permission_id) {
    auto result = get_role(role_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->permissions.erase(permission_id);
    invalidate_inheritance_cache();

    return Result::ok();
}

Result RoleManager::set_role_permissions(RoleId role_id, const std::set<PermissionId>& permissions) {
    auto result = get_role(role_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->permissions = permissions;
    invalidate_inheritance_cache();

    return Result::ok();
}

Result RoleManager::add_inheritance(RoleId child_id, RoleId parent_id) {
    auto child_result = get_role(child_id);
    auto parent_result = get_role(parent_id);

    if (!child_result || !parent_result) {
        return Result::error(ErrorCode::INVALID_PARAMETER, "Role not found");
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    child_result.value()->inherited_roles.insert(parent_id);
    invalidate_inheritance_cache();

    return Result::ok();
}

Result RoleManager::remove_inheritance(RoleId child_id, RoleId parent_id) {
    auto result = get_role(child_id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    std::lock_guard<std::shared_mutex> lock(mutex_);
    result.value()->inherited_roles.erase(parent_id);
    invalidate_inheritance_cache();

    return Result::ok();
}

ResultT<std::set<PermissionId>> RoleManager::get_all_role_permissions(RoleId role_id) {
    auto result = get_role(role_id);
    if (!result) {
        return ResultT<std::set<PermissionId>>::error(result.error_code(), result.error_message());
    }

    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (cache_dirty_) {
        build_inheritance_cache();
    }

    auto it = inheritance_cache_.find(role_id);
    if (it != inheritance_cache_.end()) {
        return ResultT<std::set<PermissionId>>::ok(it->second);
    }

    return ResultT<std::set<PermissionId>>::ok(result.value()->permissions);
}

ResultT<std::vector<RoleId>> RoleManager::get_inheritance_chain(RoleId role_id) {
    auto result = get_role(role_id);
    if (!result) {
        return ResultT<std::vector<RoleId>>::error(result.error_code(), result.error_message());
    }

    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<RoleId> chain;
    std::function<void(RoleId)> traverse = [&](RoleId id) {
        auto role = roles_.find(id);
        if (role != roles_.end()) {
            for (RoleId parent : role->second->inherited_roles) {
                traverse(parent);
            }
            chain.push_back(id);
        }
    };

    traverse(role_id);

    return ResultT<std::vector<RoleId>>::ok(chain);
}

Result RoleManager::enable_role(RoleId id) {
    auto result = get_role(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    result.value()->is_enabled = true;
    return Result::ok();
}

Result RoleManager::disable_role(RoleId id) {
    auto result = get_role(id);
    if (!result) {
        return Result::error(result.error_code(), result.error_message());
    }

    result.value()->is_enabled = false;
    return Result::ok();
}

ResultT<std::vector<Role*>> RoleManager::list_roles() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<Role*> result;
    for (const auto& pair : roles_) {
        result.push_back(pair.second.get());
    }

    return ResultT<std::vector<Role*>>::ok(result);
}

void RoleManager::build_inheritance_cache() {
    inheritance_cache_.clear();
    for (const auto& pair : roles_) {
        std::set<PermissionId> perms;
        std::function<void(RoleId)> collect = [&](RoleId id) {
            auto it = roles_.find(id);
            if (it != roles_.end()) {
                perms.insert(it->second->permissions.begin(), it->second->permissions.end());
                for (RoleId parent : it->second->inherited_roles) {
                    collect(parent);
                }
            }
        };
        collect(pair.first);
        inheritance_cache_[pair.first] = perms;
    }
    cache_dirty_ = false;
}

void RoleManager::invalidate_inheritance_cache() {
    cache_dirty_ = true;
}

RBACService& RBACService::instance() {
    static RBACService instance;
    return instance;
}

RBACService::RBACService() {
}

RBACService::~RBACService() {
}

Result RBACService::init() {
    return Result::ok();
}

void RBACService::shutdown() {
}

Result RBACService::check_permission(const std::string& session_id, PermissionId permission) {
    (void)session_id;
    (void)permission;
    return Result::ok();
}

Result RBACService::check_role(const std::string& session_id, UserRole required_role) {
    (void)session_id;
    (void)required_role;
    return Result::ok();
}

Result RBACService::check_any_permission(const std::string& session_id,
                                          const std::vector<PermissionId>& permissions) {
    (void)session_id;
    (void)permissions;
    return Result::ok();
}

Result RBACService::check_all_permissions(const std::string& session_id,
                                           const std::vector<PermissionId>& permissions) {
    (void)session_id;
    (void)permissions;
    return Result::ok();
}

Result RBACService::grant_permission_to_user(UserId user_id, PermissionId permission) {
    (void)user_id;
    (void)permission;
    return Result::ok();
}

Result RBACService::revoke_permission_from_user(UserId user_id, PermissionId permission) {
    (void)user_id;
    (void)permission;
    return Result::ok();
}

Result RBACService::grant_permission_to_role(RoleId role_id, PermissionId permission) {
    return RoleManager::instance().add_permission_to_role(role_id, permission);
}

Result RBACService::revoke_permission_from_role(RoleId role_id, PermissionId permission) {
    return RoleManager::instance().remove_permission_from_role(role_id, permission);
}

ResultT<std::set<PermissionId>> RBACService::get_user_effective_permissions(UserId user_id) {
    (void)user_id;
    return ResultT<std::set<PermissionId>>::ok(std::set<PermissionId>());
}

ResultT<std::set<RoleId>> RBACService::get_user_roles(UserId user_id) {
    (void)user_id;
    return ResultT<std::set<RoleId>>::ok(std::set<RoleId>());
}

Result RBACService::assign_role_to_user(UserId user_id, RoleId role_id) {
    (void)user_id;
    (void)role_id;
    return Result::ok();
}

Result RBACService::remove_role_from_user(UserId user_id, RoleId role_id) {
    (void)user_id;
    (void)role_id;
    return Result::ok();
}

bool RBACService::is_admin(UserId user_id) {
    (void)user_id;
    return false;
}

bool RBACService::is_vip(UserId user_id) {
    (void)user_id;
    return false;
}

bool RBACService::has_resource_access(UserId user_id, const std::string& resource,
                                        const std::string& action) {
    (void)user_id;
    (void)resource;
    (void)action;
    return true;
}

} // namespace auth
} // namespace oms
