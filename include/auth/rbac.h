#ifndef OMS_AUTH_RBAC_H
#define OMS_AUTH_RBAC_H

#include "common/types.h"
#include "common/result.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

namespace oms {
namespace auth {

struct Permission {
    PermissionId id;
    std::string name;
    std::string description;
    std::string resource;
    std::string action;
    bool is_system;
    bool is_enabled;

    bool operator==(const Permission& other) const {
        return id == other.id;
    }
};

struct Role {
    RoleId id;
    std::string name;
    std::string description;
    UserRole level;
    std::set<PermissionId> permissions;
    std::set<RoleId> inherited_roles;
    bool is_system;
    bool is_enabled;

    bool has_permission(PermissionId id) const;
    bool inherits_from(RoleId role_id) const;
};

class PermissionManager {
public:
    static PermissionManager& instance();

    Result init();
    void shutdown();

    ResultT<PermissionId> register_permission(const std::string& name,
                                                const std::string& description,
                                                const std::string& resource,
                                                const std::string& action,
                                                bool is_system = false);

    Result unregister_permission(PermissionId id);
    ResultT<Permission*> get_permission(PermissionId id);
    ResultT<Permission*> get_permission_by_name(const std::string& name);

    ResultT<std::vector<Permission*>> list_permissions();
    ResultT<std::vector<Permission*>> get_permissions_by_resource(const std::string& resource);

    Result enable_permission(PermissionId id);
    Result disable_permission(PermissionId id);

    bool has_permission(UserRole role, PermissionId permission) const;
    bool has_permission(const std::set<PermissionId>& user_perms,
                         PermissionId permission) const;

private:
    PermissionManager();
    ~PermissionManager();

    std::map<PermissionId, std::unique_ptr<Permission>> permissions_;
    std::map<std::string, PermissionId> name_index_;
    mutable std::shared_mutex mutex_;
};

class RoleManager {
public:
    static RoleManager& instance();

    Result init();
    void shutdown();

    ResultT<RoleId> create_role(const std::string& name,
                                  const std::string& description,
                                  UserRole level = UserRole::CUSTOMER);

    Result delete_role(RoleId id);
    ResultT<Role*> get_role(RoleId id);

    Result add_permission_to_role(RoleId role_id, PermissionId permission_id);
    Result remove_permission_from_role(RoleId role_id, PermissionId permission_id);
    Result set_role_permissions(RoleId role_id, const std::set<PermissionId>& permissions);

    Result add_inheritance(RoleId child_id, RoleId parent_id);
    Result remove_inheritance(RoleId child_id, RoleId parent_id);

    ResultT<std::set<PermissionId>> get_all_role_permissions(RoleId role_id);
    ResultT<std::vector<RoleId>> get_inheritance_chain(RoleId role_id);

    Result enable_role(RoleId id);
    Result disable_role(RoleId id);

    ResultT<std::vector<Role*>> list_roles();

private:
    RoleManager();
    ~RoleManager();

    void build_inheritance_cache();
    void invalidate_inheritance_cache();

    std::map<RoleId, std::unique_ptr<Role>> roles_;
    std::map<RoleId, std::set<PermissionId>> inheritance_cache_;
    bool cache_dirty_;
    mutable std::shared_mutex mutex_;
};

class RBACService {
public:
    static RBACService& instance();

    Result init();
    void shutdown();

    Result check_permission(const std::string& session_id, PermissionId permission);
    Result check_role(const std::string& session_id, UserRole required_role);
    Result check_any_permission(const std::string& session_id,
                                 const std::vector<PermissionId>& permissions);
    Result check_all_permissions(const std::string& session_id,
                                  const std::vector<PermissionId>& permissions);

    Result grant_permission_to_user(UserId user_id, PermissionId permission);
    Result revoke_permission_from_user(UserId user_id, PermissionId permission);
    Result grant_permission_to_role(RoleId role_id, PermissionId permission);
    Result revoke_permission_from_role(RoleId role_id, PermissionId permission);

    ResultT<std::set<PermissionId>> get_user_effective_permissions(UserId user_id);
    ResultT<std::set<RoleId>> get_user_roles(UserId user_id);

    Result assign_role_to_user(UserId user_id, RoleId role_id);
    Result remove_role_from_user(UserId user_id, RoleId role_id);

    bool is_admin(UserId user_id);
    bool is_vip(UserId user_id);
    bool has_resource_access(UserId user_id, const std::string& resource,
                              const std::string& action);

private:
    RBACService();
    ~RBACService();

    std::map<UserId, std::set<RoleId>> user_roles_;
    mutable std::shared_mutex mutex_;
};

// 预定义权限常量
namespace permissions {
constexpr PermissionId VIEW_DASHBOARD = 1001;
constexpr PermissionId MANAGE_USERS = 1002;
constexpr PermissionId VIEW_USERS = 1003;

constexpr PermissionId CREATE_ORDER = 2001;
constexpr PermissionId VIEW_ORDER = 2002;
constexpr PermissionId CANCEL_ORDER = 2003;
constexpr PermissionId MODIFY_ORDER = 2004;
constexpr PermissionId VIEW_ALL_ORDERS = 2005;
constexpr PermissionId EXPORT_ORDERS = 2006;

constexpr PermissionId VIEW_PRODUCT = 3001;
constexpr PermissionId CREATE_PRODUCT = 3002;
constexpr PermissionId UPDATE_PRODUCT = 3003;
constexpr PermissionId DELETE_PRODUCT = 3004;
constexpr PermissionId MANAGE_INVENTORY = 3005;

constexpr PermissionId PROCESS_PAYMENT = 4001;
constexpr PermissionId VIEW_PAYMENTS = 4002;
constexpr PermissionId REFUND_PAYMENT = 4003;

constexpr PermissionId VIEW_AUDIT_LOGS = 5001;
constexpr PermissionId EXPORT_AUDIT_LOGS = 5002;

constexpr PermissionId MANAGE_ROLES = 6001;
constexpr PermissionId MANAGE_PERMISSIONS = 6002;

constexpr PermissionId ACCESS_REPORTS = 7001;
constexpr PermissionId GENERATE_REPORTS = 7002;
constexpr PermissionId SCHEDULE_REPORTS = 7003;

constexpr PermissionId SYSTEM_CONFIG = 8001;
constexpr PermissionId VIEW_METRICS = 8002;
constexpr PermissionId MANAGE_CACHE = 8003;
}

} // namespace auth
} // namespace oms

#endif // OMS_AUTH_RBAC_H
