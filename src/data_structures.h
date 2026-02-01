#pragma once

#include "types.h"
#include <vector>
#include <deque>

namespace cl {
namespace data_feed {
namespace data_feed_parser {

struct Order {
    PAIR_ID pairId;
    ORDER_PRICE price;
    ORDER_TIME time;
    ORDER_QTY qty;
    ORDER_SIDE side;
    ORDER_TYPE type;
    ORDER_ACTION action;
};

struct bookElement {
    ORDER_PRICE price;
    ORDER_QTY qty;
    ORDER_TIME time;
};

typedef std::deque<bookElement> BookSide;

struct PairOrderBookCache {
    BookSide oldBuySide;
    BookSide newBuySide;
    BookSide oldSellSide;
    BookSide newSellSide;
};

struct SeekerBounds {
    double maxBidSeen = -MAX_DOUBLE;
    double minAskSeen = MAX_DOUBLE;
};

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
