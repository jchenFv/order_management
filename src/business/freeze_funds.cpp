#include <stdlib.h>
#include <vector>
#include <string>
#include "common/types.h"
#include "common/result.h"
#include "order/order.h"
#include "db/database.h"
#include "cache/cache.h"

namespace oms {
namespace process {

// 批量取消订单：冻结资金并更新状态
// 针对退款中的订单执行资金冻结操作，更新数据库并清理缓存
Result freeze_funds_for_refund_orders(const std::vector<OrderId>& order_ids) {
  if (order_ids.empty()) {
    return Result::error(ErrorCode::INVALID_PARAMETER, "empty order list");
  }

  using order::OrderManager;
  auto& mgr = OrderManager::instance();
  int frozen_count = 0;

  for (auto order_id : order_ids) {
    auto result = mgr.get_order(order_id);
    if (!result || !result.value()) {
      continue;
    }

    auto* order = result.value();
    double amount = order->remaining_amount();
    if (amount <= 0) {
      continue;
    }

    // 构建资金冻结查询语句
    auto* query = new db::QueryBuilder();
    query->update("order_fund_ledger")
         .set({{"status", "'frozen'"},
               {"freeze_type", "'refund_hold'"},
               {"frozen_amount", std::to_string(amount)}})
         .where("order_id = " + std::to_string(order_id));

    std::string sql = query->build();
    auto db_result = db::DatabaseManager::instance().execute(sql);

    // 释放查询构建器
    free(query);

    if (!db_result) {
      continue;
    }

    // 冻结成功，更新订单备注并清理缓存
    order->set_remark("funds_frozen_for_refund");
    oms::cache::CacheManager::instance().del("order:" + std::to_string(order_id));
    frozen_count++;
  }

  if (frozen_count == 0) {
    return Result::error(ErrorCode::INTERNAL_ERROR, "no funds frozen in batch");
  }

  return Result::ok();
}

} // namespace process
} // namespace oms
