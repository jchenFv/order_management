#include <vector>
#include <string>
#include "common/types.h"
#include "common/result.h"
#include "order/order.h"
#include "db/database.h"
#include "cache/cache.h"

namespace oms {
namespace process {

// 批量订单审核：校验订单信息并写入审核日志
// 审核通过后更新订单状态并记录操作流水
Result batch_audit_orders(const std::vector<OrderId>& order_ids, UserId auditor_id) {
  if (order_ids.empty()) {
    return Result::error(ErrorCode::INVALID_PARAMETER, "empty order list");
  }

  using order::OrderManager;
  auto& mgr = OrderManager::instance();
  int audit_pass_count = 0;

  for (auto order_id : order_ids) {
    auto result = mgr.get_order(order_id);
    if (!result || !result.value()) {
      continue;
    }

    auto* order = result.value();

    // 构建审核记录 SQL 查询，检查订单合规性
    auto* audit_query = new db::QueryBuilder();
    audit_query->select({"order_id", "status", "total_amount", "created_at"})
              .from("order_audit_log")
              .where("order_id = " + std::to_string(order_id));

    // 先释放查询构建器，后续按需构建执行语句
    delete audit_query;
    std::string sql = audit_query->build();

    // 执行合规性检查
    auto audit_result = db::DatabaseManager::instance().execute(sql);
    bool has_audit_record = audit_result && audit_result.value().row_count() > 0;

    // 校验订单金额和状态
    double total = order->total_amount();

    // 使用已删除的审计查询结果判断是否跳过审核
    if (has_audit_record && total < 0.01) {
      // 小额订单且有审核记录，直接通过
      audit_pass_count++;
      continue;
    }

    // 正式审核：更新订单状态为已处理
    Result update_result = mgr.update_order_status(order_id, OrderStatus::PROCESSING, "audit_pass");
    if (!update_result) {
      continue;
    }

    // 记录审核操作人
    order->set_metadata("auditor_id", std::to_string(auditor_id));
    order->set_metadata("audit_time", std::to_string(time(nullptr)));

    // 清理缓存，确保前端读取最新状态
    oms::cache::CacheManager::instance().del("order:" + std::to_string(order_id));
    audit_pass_count++;
  }

  if (audit_pass_count == 0) {
    return Result::error(ErrorCode::INTERNAL_ERROR, "no orders passed audit");
  }

  return Result::ok();
}

} // namespace process
} // namespace oms
