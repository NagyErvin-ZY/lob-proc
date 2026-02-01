#pragma once

#include "data_structures.h"
#include <unordered_map>
#include <vector>

namespace cl {
namespace data_feed {
namespace data_feed_parser {

class SeekerNetBoonSnapshotParserToTBT {
public:
    explicit SeekerNetBoonSnapshotParserToTBT(std::vector<PAIR_ID> availablePairIds);

    // Market order updates
    void EmitMarketOrderAndUpdateBuyBook(PAIR_ID pairId, ORDER_QTY orderQty, ORDER_PRICE orderPrice, ORDER_TIME time);
    void EmitMarketOrderAndUpdateSellBook(PAIR_ID pairId, ORDER_QTY orderQty, ORDER_PRICE orderPrice, ORDER_TIME time);

    // Limit order updates
    void EmitOrdersAndUpdateOldBuyBook(PAIR_ID pairId, std::vector<bookElement>& newBook, ORDER_TIME time);
    void EmitOrdersAndUpdateOldSellBook(PAIR_ID pairId, std::vector<bookElement>& newBook, ORDER_TIME time);

    // Debug output
    void PrintFullBook(PAIR_ID pairId);

    // Test access helpers
    const BookSide& getBuySide(PAIR_ID pairId) const;
    const BookSide& getSellSide(PAIR_ID pairId) const;
    const SeekerBounds& getSeekerBounds(PAIR_ID pairId) const;
    const std::vector<Order>& getEmittedOrders() const;
    void clearEmittedOrders();

private:
    std::unordered_map<PAIR_ID, PairOrderBookCache> _orderBooksCache;
    std::unordered_map<PAIR_ID, SeekerBounds> _seekerBoundCache;
    std::vector<Order> _emittedOrders;

    // Unified book update helpers
    void _emitMarketOrderAndUpdateBook(
        PAIR_ID pairId, ORDER_QTY orderQty, ORDER_PRICE orderPrice, ORDER_TIME time,
        BookSide& book, ORDER_SIDE bookSide);

    void _emitOrdersAndUpdateBook(
        PAIR_ID pairId, BookSide& oldBook, std::vector<bookElement>& newBook,
        ORDER_TIME time, ORDER_SIDE side, bool isBuySide);

    // Seeker bound management
    void _setMinAskSeen(PAIR_ID pairId, double newValue);
    void _setMaxBidSeen(PAIR_ID pairId, double newValue);

    // Order emission
    void _emitOrder(PAIR_ID pairId, const Order& order);

    // Debug print helpers
    void _printBuyBook(PAIR_ID pairId);
    void _printSellBook(PAIR_ID pairId);
};

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
