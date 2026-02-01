#include "utils.h"

namespace cl {
namespace data_feed {
namespace data_feed_parser {

const char* toString(ORDER_SIDE side) {
    switch (side) {
        case ORDER_SIDE::BUY:  return "BUY";
        case ORDER_SIDE::SELL: return "SELL";
        default:               return "N/A";
    }
}

const char* toString(ORDER_TYPE type) {
    switch (type) {
        case ORDER_TYPE::LIMIT:   return "LIMIT";
        case ORDER_TYPE::MARKET:  return "MARKET";
        case ORDER_TYPE::ICEBERG: return "ICEBERG";
        case ORDER_TYPE::STOP:    return "STOP";
        default:                  return "N/A";
    }
}

const char* toString(ORDER_ACTION action) {
    switch (action) {
        case ORDER_ACTION::SEEKER_ADD: return "SEEKER_ADD";
        case ORDER_ACTION::ADD:        return "ADD";
        case ORDER_ACTION::REMOVE:     return "REMOVE";
        case ORDER_ACTION::MODIFY:     return "MODIFY";
        default:                       return "N/A";
    }
}

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
