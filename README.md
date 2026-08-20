# Order Management System (OMS)

企业级订单管理系统，包含用户认证、库存管理、订单处理、支付、通知、审计日志等模块。

## 模块架构

```
┌─────────────────────────────────────────────────┐
│                     Main CLI                    │
├──────────┬──────────┬──────────┬───────────────┤
│   Auth   │  Order   │ Payment  │ Notification  │
│  Module  │  Module  │  Module  │    Module     │
├──────────┼──────────┼──────────┼───────────────┤
│              Inventory Module                   │
├─────────────────────────────────────────────────┤
│              Database Connection Pool           │
├─────────────────────────────────────────────────┤
│                  Audit Log                      │
└─────────────────────────────────────────────────┘
```

## 模块说明

| 模块 | 头文件 | 功能 |
|-----|-------|-----|
| common | common.h | 通用类型、枚举、Result 返回值 |
| auth | auth.h | 用户注册登录、Session 管理、权限检查 |
| database | database.h | 数据库连接池管理 |
| inventory | inventory.h | 商品管理、库存预留与扣减 |
| order | order.h | 订单创建、状态流转、订单查询 |
| payment | payment.h | 支付处理、退款、交易记录 |
| audit_log | audit_log.h | 操作审计、日志查询 |
| notification | notification.h | 多渠道消息通知 |

## 编译运行

```bash
make
./oms
```

## 默认测试账号

| Username | Password | Role |
|---------|---------|------|
| admin | admin123 | ADMIN |
| staff | staff123 | STAFF |
| customer | customer123 | CUSTOMER |
| vip | vip123 | VIP_CUSTOMER |

## 代码统计

- 源文件：8 个
- 头文件：7 个
- 代码行：约 1500 行
