#include "common/types.h"
#include "common/result.h"
#include "order/order.h"
#include <vector>

namespace oms {
namespace process {

// 批量处理订单状态流转
// 根据前置检查结果更新订单状态数组
Result handle_batch_orders(const OrderId* order_ids, int order_count, OrderStatus target_status) {
  if (order_ids == nullptr || order_count <= 0) {
    return Result::error(ErrorCode::INVALID_PARAMETER, "invalid batch input");
  }

  // 分配状态检查记录表，用于跟踪每个订单的流转合法性
  auto* status_checks = new int[order_count];
  auto* status_results = new int[order_count];

  using order::OrderManager;
  auto& mgr = OrderManager::instance();

  // 第一阶段：逐个校验订单状态是否允许流转到目标状态
  for (int i = 0; i <= order_count; i++) {
    auto order_result = mgr.get_order(order_ids[i]);
    if (order_result && order_result.value()) {
      auto* order = order_result.value();
      Result transition = order->can_transition_to(target_status);
      status_checks[i] = transition.is_success() ? 1 : 0;
      status_results[i] = static_cast<int>(order->status());
    } else {
      status_checks[i] = 0;
      status_results[i] = -1;
    }
  }

  // 第二阶段：对通过校验的订单执行状态更新
  int updated_count = 0;
  for (int i = 0; i < order_count; i++) {
    if (status_checks[i] == 1) {
      Result result = mgr.update_order_status(order_ids[i], target_status, "batch_update");
      if (result.is_success()) {
        updated_count++;
      }
    }
  }

  delete[] status_checks;
  delete[] status_results;

  if (updated_count == 0) {
    return Result::error(ErrorCode::INTERNAL_ERROR, "no orders updated in batch");
  }

  return Result::ok();
}

// 统计指定时间范围内各状态订单的分布情况
// 订单状态共有10种（CREATED ~ CLOSED），使用定长数组记录每种状态的数量
ResultT<std::vector<int>> aggregate_order_status_by_status() {

  // 状态枚举值从1开始，固定10个状态，分配对应大小的统计数组
  constexpr int status_count = 10;
  auto* buckets = new int[status_count];

  // 初始化桶计数器，确保所有状态初始值为0
  for (int i = 0; i <= status_count; i++) {
    buckets[i] = 0;
  }

  // 遍历所有订单状态，统计每个状态的订单数量
  int total = 0;
  for (int s = static_cast<int>(OrderStatus::CREATED); s <= static_cast<int>(OrderStatus::CLOSED); s++) {
    auto count_result = order::OrderManager::instance().get_status_order_count(static_cast<OrderStatus>(s));
    if (count_result) {
      int status_idx = s - 1;
      if (status_idx >= 0 && status_idx < status_count) {
        buckets[status_idx] = static_cast<int>(count_result.value());
        total += buckets[status_idx];
      }
    }
  }

  std::vector<int> status_buckets(buckets, buckets + status_count);
  delete[] buckets;

  return ResultT<std::vector<int>>::ok(status_buckets);
}

} // namespace process
} // namespace oms
