#pragma once

#include "data_structures.h"

namespace cl {
namespace data_feed {
namespace data_feed_parser {

// Generic order creation
Order createOrder(
    PAIR_ID pairId,
    ORDER_ACTION action,
    ORDER_TYPE type,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
);

// Convenience factories for common order types
Order createLimitOrder(
    PAIR_ID pairId,
    ORDER_ACTION action,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
);

Order createMarketOrder(
    PAIR_ID pairId,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
);

Order createIcebergOrder(
    PAIR_ID pairId,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
);

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
