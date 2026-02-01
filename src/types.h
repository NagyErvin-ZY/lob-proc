#pragma once

#include <limits>
#include <cstdint>

namespace cl {
namespace data_feed {
namespace data_feed_parser {

// Type definitions
typedef int64_t PAIR_ID;
typedef double ORDER_PRICE;
typedef uint64_t ORDER_TIME;
typedef int32_t ORDER_QTY;

// Constants
constexpr double DoubleComparisonEpsilon = 1e-5;
constexpr double MAX_DOUBLE = std::numeric_limits<double>::max();

// Enums
enum ORDER_SIDE {
    BUY = 1,
    SELL = 2
};

enum ORDER_TYPE {
    LIMIT = 1,
    MARKET = 2,
    ICEBERG = 3,
    STOP = 4
};

enum ORDER_ACTION {
    SEEKER_ADD = 0,
    ADD = 1,
    REMOVE = 2,
    MODIFY = 3
};

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
