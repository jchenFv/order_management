#ifndef OMS_H
#define OMS_H

#include "common/types.h"
#include "common/result.h"
#include "common/config.h"

#include "auth/user.h"
#include "auth/session.h"
#include "auth/rbac.h"

#include "order/order_item.h"
#include "order/order.h"
#include "order/discount.h"

#include "inventory/product.h"
#include "inventory/warehouse.h"

#include "payment/payment.h"
#include "db/database.h"
#include "cache/cache.h"

namespace oms {

Result initialize();
void shutdown();

const char* version();
const char* build_timestamp();

} // namespace oms

#endif // OMS_H
