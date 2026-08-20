#include "oms.h"
#include "auth/user.h"
#include "auth/session.h"
#include "auth/rbac.h"
#include "order/order.h"
#include "inventory/product.h"
#include "inventory/warehouse.h"
#include "common/config.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace oms;

void print_banner() {
    std::cout << "\n";
    std::cout << "  ██████╗ ██████╗ ███████╗    ████████╗███████╗ █████╗ ███╗   ███╗\n";
    std::cout << "  ██╔══██╗██╔══██╗██╔════╝    ╚══██╔══╝██╔════╝██╔══██╗████╗ ████║\n";
    std::cout << "  ██████╔╝██████╔╝███████╗       ██║   █████╗  ███████║██╔████╔██║\n";
    std::cout << "  ██╔═══╝ ██╔══██╗╚════██║       ██║   ██╔══╝  ██╔══██║██║╚██╔╝██║\n";
    std::cout << "  ██║     ██║  ██║███████║       ██║   ███████╗██║  ██║██║ ╚═╝ ██║\n";
    std::cout << "  ╚═╝     ╚═╝  ╚═╝╚══════╝       ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝\n";
    std::cout << "\n";
    std::cout << "  Order Management System v1.0.0\n";
    std::cout << "  ==================================\n";
    std::cout << "\n";
}

void print_menu() {
    std::cout << "\n";
    std::cout << "  [1] User Management\n";
    std::cout << "  [2] Product Management\n";
    std::cout << "  [3] Order Management\n";
    std::cout << "  [4] Inventory Management\n";
    std::cout << "  [5] Discount & Promotion\n";
    std::cout << "  [6] System Status\n";
    std::cout << "  [0] Exit\n";
    std::cout << "\n";
    std::cout << "  Select option: ";
}

void print_user_menu() {
    std::cout << "\n";
    std::cout << "  --- User Management ---\n";
    std::cout << "  [1] List all users\n";
    std::cout << "  [2] Create user\n";
    std::cout << "  [3] User details\n";
    std::cout << "  [4] Update user role\n";
    std::cout << "  [5] Grant permission\n";
    std::cout << "  [6] Revoke permission\n";
    std::cout << "  [0] Back to main\n";
    std::cout << "\n";
    std::cout << "  Select option: ";
}

void print_product_menu() {
    std::cout << "\n";
    std::cout << "  --- Product Management ---\n";
    std::cout << "  [1] List all products\n";
    std::cout << "  [2] Create product\n";
    std::cout << "  [3] Product details\n";
    std::cout << "  [4] Update price\n";
    std::cout << "  [5] Update stock\n";
    std::cout << "  [6] Search products\n";
    std::cout << "  [0] Back to main\n";
    std::cout << "\n";
    std::cout << "  Select option: ";
}

void print_order_menu() {
    std::cout << "\n";
    std::cout << "  --- Order Management ---\n";
    std::cout << "  [1] List all orders\n";
    std::cout << "  [2] Create order\n";
    std::cout << "  [3] Order details\n";
    std::cout << "  [4] Update order status\n";
    std::cout << "  [5] Cancel order\n";
    std::cout << "  [6] Apply discount\n";
    std::cout << "  [0] Back to main\n";
    std::cout << "\n";
    std::cout << "  Select option: ";
}

Result initialize_all_modules() {
    Result r;

    r = config::load_global_config();
    if (!r) {
        std::cerr << "  [ERROR] Failed to load config: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] Config loaded\n";

    r = auth::UserManager::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init UserManager: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] User Manager initialized\n";

    r = auth::SessionManager::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init SessionManager: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] Session Manager initialized\n";

    r = auth::RBACService::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init RBACService: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] RBAC Service initialized\n";

    r = inventory::ProductCatalog::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init ProductCatalog: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] Product Catalog initialized\n";

    r = inventory::WarehouseManager::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init WarehouseManager: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] Warehouse Manager initialized\n";

    r = order::OrderManager::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init OrderManager: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] Order Manager initialized\n";

    r = order::DiscountManager::instance().init();
    if (!r) {
        std::cerr << "  [ERROR] Failed to init DiscountManager: " << r.error_message() << "\n";
        return r;
    }
    std::cout << "  [OK] Discount Manager initialized\n";

    return Result::ok();
}

void create_sample_users() {
    auto& um = auth::UserManager::instance();

    um.create_user("admin", "admin123", UserRole::ADMIN);
    um.create_user("staff", "staff123", UserRole::STAFF);
    um.create_user("manager", "manager123", UserRole::MANAGER);
    um.create_user("customer1", "cust123", UserRole::CUSTOMER);
    um.create_user("vip_customer", "vip123", UserRole::VIP_CUSTOMER);

    std::cout << "  [OK] Sample users created\n";
}

void create_sample_products() {
    auto& pc = inventory::ProductCatalog::instance();

    pc.add_product("iPhone 15 Pro", "IP15P-256", 8999.0, 100);
    pc.add_product("MacBook Pro 14", "MBP14-M3", 14999.0, 50);
    pc.add_product("AirPods Pro 2", "APP2", 1899.0, 200);
    pc.add_product("iPad Air", "IPA-M2", 4799.0, 80);
    pc.add_product("Apple Watch Ultra 2", "AWU2", 6499.0, 60);
    pc.add_product("Magic Keyboard", "MK-BT", 699.0, 150);
    pc.add_product("Studio Display", "SD-27", 11499.0, 30);
    pc.add_product("HomePod 2", "HP2", 2299.0, 100);

    std::cout << "  [OK] Sample products created\n";
}

void create_sample_warehouses() {
    auto& wm = inventory::WarehouseManager::instance();

    wm.create_warehouse("Main Warehouse - Shanghai", "WH-SH-01");
    wm.create_warehouse("East Warehouse - Shenzhen", "WH-SZ-01");
    wm.create_warehouse("North Warehouse - Beijing", "WH-BJ-01");
    wm.create_warehouse("West Warehouse - Chengdu", "WH-CD-01");

    wm.set_default_warehouse(1);

    std::cout << "  [OK] Sample warehouses created\n";
}

void handle_user_management() {
    int choice;
    do {
        print_user_menu();
        std::cin >> choice;

        switch (choice) {
            case 1: {
                auto result = auth::UserManager::instance().list_users(0, 50);
                if (result) {
                    std::cout << "\n  --- User List ---\n";
                    std::cout << std::setw(6) << "ID"
                              << std::setw(20) << "Username"
                              << std::setw(15) << "Role"
                              << std::setw(10) << "Active\n";
                    std::cout << "  ----------------------------------------------------\n";
                    for (auto user : result.value()) {
                        std::string role_str;
                        switch (user->role()) {
                            case UserRole::ADMIN: role_str = "ADMIN"; break;
                            case UserRole::MANAGER: role_str = "MANAGER"; break;
                            case UserRole::STAFF: role_str = "STAFF"; break;
                            case UserRole::VIP_CUSTOMER: role_str = "VIP"; break;
                            case UserRole::CUSTOMER: role_str = "CUSTOMER"; break;
                            default: role_str = "GUEST"; break;
                        }
                        std::cout << "  " << std::setw(4) << user->id()
                                  << std::setw(20) << user->username()
                                  << std::setw(15) << role_str
                                  << std::setw(8) << (user->is_active() ? "Yes" : "No") << "\n";
                    }
                }
                break;
            }
            case 2: {
                std::string username, password;
                int role;
                std::cout << "  Username: ";
                std::cin >> username;
                std::cout << "  Password: ";
                std::cin >> password;
                std::cout << "  Role (1=Customer,2=VIP,3=Staff,4=Manager,5=Admin): ";
                std::cin >> role;

                auto r = auth::UserManager::instance().create_user(
                    username, password, static_cast<UserRole>(role));
                if (r) {
                    std::cout << "  [OK] User created: ID=" << r.value()->id() << "\n";
                } else {
                    std::cout << "  [ERROR] " << r.error_message() << "\n";
                }
                break;
            }
            case 3: {
                UserId id;
                std::cout << "  User ID: ";
                std::cin >> id;
                auto r = auth::UserManager::instance().get_user(id);
                if (r) {
                    auto user = r.value();
                    std::cout << "\n  --- User Details ---\n";
                    std::cout << "  ID: " << user->id() << "\n";
                    std::cout << "  Username: " << user->username() << "\n";
                    std::cout << "  Role: " << static_cast<int>(user->role()) << "\n";
                    std::cout << "  Active: " << (user->is_active() ? "Yes" : "No") << "\n";
                    std::cout << "  Permissions: " << user->permissions().size() << "\n";
                } else {
                    std::cout << "  [ERROR] User not found\n";
                }
                break;
            }
            default:
                break;
        }
    } while (choice != 0);
}

void handle_product_management() {
    int choice;
    do {
        print_product_menu();
        std::cin >> choice;

        switch (choice) {
            case 1: {
                auto result = inventory::ProductCatalog::instance().list_products(0, 50);
                if (result) {
                    std::cout << "\n  --- Product List ---\n";
                    std::cout << std::setw(8) << "ID"
                              << std::setw(25) << "Name"
                              << std::setw(15) << "SKU"
                              << std::setw(12) << "Price"
                              << std::setw(10) << "Stock\n";
                    std::cout << "  --------------------------------------------------------------------\n";
                    for (auto product : result.value()) {
                        std::cout << "  " << std::setw(6) << product->id()
                                  << std::setw(25) << product->name()
                                  << std::setw(15) << product->sku()
                                  << std::setw(12) << std::fixed << std::setprecision(2) << product->price()
                                  << std::setw(8) << product->total_stock() << "\n";
                    }
                }
                break;
            }
            case 2: {
                std::string name, sku;
                double price;
                int stock;
                std::cout << "  Product name: ";
                std::cin.ignore();
                std::getline(std::cin, name);
                std::cout << "  SKU: ";
                std::cin >> sku;
                std::cout << "  Price: ";
                std::cin >> price;
                std::cout << "  Initial stock: ";
                std::cin >> stock;

                auto r = inventory::ProductCatalog::instance().add_product(name, sku, price, stock);
                if (r) {
                    std::cout << "  [OK] Product created: ID=" << r.value() << "\n";
                } else {
                    std::cout << "  [ERROR] " << r.error_message() << "\n";
                }
                break;
            }
            default:
                break;
        }
    } while (choice != 0);
}

void handle_order_management() {
    int choice;
    do {
        print_order_menu();
        std::cin >> choice;
    } while (choice != 0);
}

void print_system_status() {
    std::cout << "\n  --- System Status ---\n";

    auto user_count = auth::UserManager::instance().get_user_count();
    auto product_count = inventory::ProductCatalog::instance().get_total_product_count();

    std::cout << "  User count: " << (user_count ? user_count.value() : 0) << "\n";
    std::cout << "  Product count: " << (product_count ? product_count.value() : 0) << "\n";

    auto& config = config::global_config();
    std::cout << "  Server port: " << config.server().port << "\n";
    std::cout << "  DB host: " << config.database().host << ":" << config.database().port << "\n";
    std::cout << "  Redis: " << config.redis().host << ":" << config.redis().port << "\n";
    std::cout << "  Session timeout: " << config.security().session_timeout_sec << "s\n";
    std::cout << "  Audit log enabled: " << (config.audit().enable_audit_log ? "Yes" : "No") << "\n";

    std::cout << "\n  All systems operational!\n";
}

int main() {
    print_banner();

    std::cout << "  Initializing modules...\n\n";
    Result r = initialize_all_modules();
    if (!r) {
        std::cerr << "\n  [FATAL] Failed to initialize system!\n";
        return 1;
    }

    std::cout << "\n  Creating sample data...\n";
    create_sample_users();
    create_sample_products();
    create_sample_warehouses();

    std::cout << "\n  ==================================\n";
    std::cout << "  System ready! Type 'help' for commands.\n";

    int choice;
    do {
        print_menu();
        std::cin >> choice;

        switch (choice) {
            case 1:
                handle_user_management();
                break;
            case 2:
                handle_product_management();
                break;
            case 3:
                handle_order_management();
                break;
            case 6:
                print_system_status();
                break;
            case 0:
                std::cout << "\n  Shutting down...\n";
                auth::UserManager::instance().shutdown();
                auth::SessionManager::instance().shutdown();
                auth::RBACService::instance().shutdown();
                inventory::ProductCatalog::instance().shutdown();
                inventory::WarehouseManager::instance().shutdown();
                order::OrderManager::instance().shutdown();
                order::DiscountManager::instance().shutdown();
                std::cout << "  Goodbye!\n";
                break;
            default:
                std::cout << "  Invalid option\n";
                break;
        }
    } while (choice != 0);

    return 0;
}
