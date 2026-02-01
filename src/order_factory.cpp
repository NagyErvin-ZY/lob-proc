#include "order_factory.h"

namespace cl {
namespace data_feed {
namespace data_feed_parser {

Order createOrder(
    PAIR_ID pairId,
    ORDER_ACTION action,
    ORDER_TYPE type,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
) {
    Order order;
    order.pairId = pairId;
    order.action = action;
    order.type = type;
    order.price = price;
    order.qty = qty;
    order.side = side;
    order.time = time;
    return order;
}

Order createLimitOrder(
    PAIR_ID pairId,
    ORDER_ACTION action,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
) {
    return createOrder(pairId, action, ORDER_TYPE::LIMIT, price, qty, side, time);
}

Order createMarketOrder(
    PAIR_ID pairId,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
) {
    return createOrder(pairId, ORDER_ACTION::ADD, ORDER_TYPE::MARKET, price, qty, side, time);
}

Order createIcebergOrder(
    PAIR_ID pairId,
    ORDER_PRICE price,
    ORDER_QTY qty,
    ORDER_SIDE side,
    ORDER_TIME time
) {
    return createOrder(pairId, ORDER_ACTION::ADD, ORDER_TYPE::ICEBERG, price, qty, side, time);
}

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
