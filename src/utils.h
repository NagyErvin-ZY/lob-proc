#pragma once

#include "types.h"
#include <cmath>

namespace cl {
namespace data_feed {
namespace data_feed_parser {

// Safe double comparison with epsilon
inline bool SafeDoubleCompare(double first, double second) {
    return std::abs(first - second) < DoubleComparisonEpsilon;
}

// Custom comparator for doubles (modern functor, no deprecated binary_function)
class CustomDoubleComparator {
public:
    explicit CustomDoubleComparator(double epsilon = DoubleComparisonEpsilon)
        : _epsilon(epsilon) {}

    bool operator()(double left, double right) const {
        return (std::abs(left - right) > _epsilon) && (left < right);
    }

private:
    double _epsilon;
};

// Enum to string conversions
const char* toString(ORDER_SIDE side);
const char* toString(ORDER_TYPE type);
const char* toString(ORDER_ACTION action);

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
