#pragma once

#include <gtest/gtest.h>
#include "order_book_parser.h"

using namespace cl::data_feed::data_feed_parser;

inline bookElement makeBookElement(double price, int qty, ORDER_TIME time = 0) {
    bookElement e;
    e.price = price;
    e.qty = qty;
    e.time = time;
    return e;
}
