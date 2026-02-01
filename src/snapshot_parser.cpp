#include "snapshot_parser.h"
#include "order_factory.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

namespace cl {
namespace data_feed {
namespace data_feed_parser {

SeekerNetBoonSnapshotParserToTBT::SeekerNetBoonSnapshotParserToTBT(std::vector<PAIR_ID> availablePairIds) {
    _emittedOrders.reserve(256);
    for (auto& pairId : availablePairIds) {
        _orderBooksCache.insert({pairId, PairOrderBookCache{}});
        SeekerBounds bounds;
        bounds.maxBidSeen = -MAX_DOUBLE;
        bounds.minAskSeen = MAX_DOUBLE;
        _seekerBoundCache.insert({pairId, bounds});
    }
}

const BookSide& SeekerNetBoonSnapshotParserToTBT::getBuySide(PAIR_ID pairId) const {
    return _orderBooksCache.at(pairId).oldBuySide;
}

const BookSide& SeekerNetBoonSnapshotParserToTBT::getSellSide(PAIR_ID pairId) const {
    return _orderBooksCache.at(pairId).oldSellSide;
}

const SeekerBounds& SeekerNetBoonSnapshotParserToTBT::getSeekerBounds(PAIR_ID pairId) const {
    return _seekerBoundCache.at(pairId);
}

const std::vector<Order>& SeekerNetBoonSnapshotParserToTBT::getEmittedOrders() const {
    return _emittedOrders;
}

void SeekerNetBoonSnapshotParserToTBT::clearEmittedOrders() {
    _emittedOrders.clear();
}

void SeekerNetBoonSnapshotParserToTBT::_emitMarketOrderAndUpdateBook(
    PAIR_ID pairId, ORDER_QTY orderQty, ORDER_PRICE orderPrice, ORDER_TIME time,
    BookSide& oldBook, ORDER_SIDE bookSide
) {
    ORDER_SIDE oppositeSide = (bookSide == ORDER_SIDE::BUY) ? ORDER_SIDE::SELL : ORDER_SIDE::BUY;

    if (oldBook.size() == 0) {
        _emittedOrders.push_back({pairId, orderPrice, time, orderQty, bookSide, ORDER_TYPE::ICEBERG, ORDER_ACTION::ADD});
        _emittedOrders.push_back({pairId, orderPrice, time, orderQty, oppositeSide, ORDER_TYPE::MARKET, ORDER_ACTION::ADD});
    }
    else if (SafeDoubleCompare(oldBook.begin()->price, orderPrice)) {
        auto qtyDifference = oldBook.begin()->qty - orderQty;
        if (qtyDifference > 0) {
            oldBook.begin()->qty = qtyDifference;
            oldBook.begin()->time = time;
            _emittedOrders.push_back({pairId, orderPrice, time, orderQty, oppositeSide, ORDER_TYPE::MARKET, ORDER_ACTION::ADD});
        }
        else if (qtyDifference == 0) {
            _emittedOrders.push_back({pairId, orderPrice, time, orderQty, oppositeSide, ORDER_TYPE::MARKET, ORDER_ACTION::ADD});
            oldBook.erase(oldBook.begin());
        }
        else {
            _emittedOrders.push_back({pairId, orderPrice, time, -qtyDifference, bookSide, ORDER_TYPE::ICEBERG, ORDER_ACTION::ADD});
            _emittedOrders.push_back({pairId, orderPrice, time, orderQty, oppositeSide, ORDER_TYPE::MARKET, ORDER_ACTION::ADD});
            oldBook.erase(oldBook.begin());
        }
    }
    else if (bookSide == ORDER_SIDE::BUY) {
        std::cout << "Error on buyside book at PAIRID=" << pairId
                  << " no matching liquidty at price=" << orderPrice << std::endl;
    }
}

void SeekerNetBoonSnapshotParserToTBT::EmitMarketOrderAndUpdateBuyBook(
    PAIR_ID pairId, ORDER_QTY orderQty, ORDER_PRICE orderPrice, ORDER_TIME time
) {
    _emitMarketOrderAndUpdateBook(pairId, orderQty, orderPrice, time,
        _orderBooksCache.at(pairId).oldBuySide, ORDER_SIDE::BUY);
}

void SeekerNetBoonSnapshotParserToTBT::EmitMarketOrderAndUpdateSellBook(
    PAIR_ID pairId, ORDER_QTY orderQty, ORDER_PRICE orderPrice, ORDER_TIME time
) {
    _emitMarketOrderAndUpdateBook(pairId, orderQty, orderPrice, time,
        _orderBooksCache.at(pairId).oldSellSide, ORDER_SIDE::SELL);
}

void SeekerNetBoonSnapshotParserToTBT::_emitOrdersAndUpdateBook(
    PAIR_ID pairId, BookSide& oldBook, std::vector<bookElement>& newBook,
    ORDER_TIME time, ORDER_SIDE side, bool isBuySide
) {
    SeekerBounds& bounds = _seekerBoundCache.at(pairId);
    double defaultPrice = isBuySide ? 0 : MAX_DOUBLE;

    auto checkAndUpdateSeeker = [&](double price) -> ORDER_ACTION {
        if (isBuySide) {
            if (price > bounds.maxBidSeen) {
                bounds.maxBidSeen = price;
                return ORDER_ACTION::SEEKER_ADD;
            }
        } else {
            if (price < bounds.minAskSeen) {
                bounds.minAskSeen = price;
                return ORDER_ACTION::SEEKER_ADD;
            }
        }
        return ORDER_ACTION::ADD;
    };

    auto priceIsBetter = [&](double a, double b) -> bool {
        return isBuySide ? (a > b) : (a < b);
    };

    // Helper to emit a limit order directly into the vector
    auto emitLimit = [&](PAIR_ID pid, ORDER_ACTION action, ORDER_PRICE price, ORDER_QTY qty) {
        _emittedOrders.push_back({pid, price, time, qty, side, ORDER_TYPE::LIMIT, action});
    };

    if (newBook.size() == 0 && oldBook.size() == 0) {
        return;
    }
    else if (newBook.size() == 0) {
        do {
            auto& back = *(oldBook.end() - 1);
            emitLimit(pairId, ORDER_ACTION::REMOVE, back.price, back.qty);
            oldBook.pop_back();
        } while (oldBook.size() > 0);
    }
    else if (oldBook.size() == 0) {
        for (auto iter = newBook.begin(); iter != newBook.end(); iter++) {
            bookElement tmp;
            tmp.price = iter->price;
            tmp.qty = iter->qty;
            oldBook.push_back(tmp);

            ORDER_ACTION action = checkAndUpdateSeeker(iter->price);
            emitLimit(pairId, action, iter->price, iter->qty);
        }
    }
    else {
        if (isBuySide && oldBook.size() > 0 && newBook.size() > 0 &&
            SafeDoubleCompare(oldBook[0].price, newBook[0].price)) {
            auto qtyDifference = newBook[0].qty - oldBook[0].qty;
            if (qtyDifference > 0) {
                oldBook[0].qty += qtyDifference;
                emitLimit(pairId, ORDER_ACTION::ADD, oldBook[0].price, qtyDifference);
            }
            else if (qtyDifference < 0) {
                oldBook[0].qty += qtyDifference;
                emitLimit(pairId, ORDER_ACTION::REMOVE, oldBook[0].price, -qtyDifference);
            }
        }

        long loopEnd = isBuySide ? static_cast<long>(newBook.size())
                                 : static_cast<long>(std::max(newBook.size(), oldBook.size()));

        for (int i = 1; i < loopEnd; i++) {
            double oldBookPriceLevel;
            double newBookPriceLevel;
            double nextOldBookPriceLevel;
            double nextNewBookPriceLevel;
            long maxIterations = static_cast<long>(oldBook.size() + newBook.size()) * 4 + 16;

            do {
                if (--maxIterations <= 0) {
                    std::cerr << "Seeker diff exceeded iteration guard for pairId=" << pairId
                              << " (old=" << oldBook.size() << ", new=" << newBook.size() << ")\n";
                    break;
                }
                if (oldBook.empty() || newBook.empty()) break;

                long oldBookSize = static_cast<long>(oldBook.size()) - 1;
                long newBookSize = static_cast<long>(newBook.size()) - 1;

                if (oldBookSize >= i - 1) { oldBookPriceLevel = oldBook[i - 1].price; }
                else { oldBookPriceLevel = defaultPrice; }
                if (newBookSize >= i - 1) { newBookPriceLevel = newBook[i - 1].price; }
                else { newBookPriceLevel = defaultPrice; }
                if (oldBookSize >= i) { nextOldBookPriceLevel = oldBook[i].price; }
                else { nextOldBookPriceLevel = defaultPrice; }
                if (newBookSize >= i) { nextNewBookPriceLevel = newBook[i].price; }
                else { nextNewBookPriceLevel = defaultPrice; }

                if (priceIsBetter(oldBookPriceLevel, newBookPriceLevel)) {
                    emitLimit(pairId, ORDER_ACTION::REMOVE, oldBook.front().price, oldBook.front().qty);
                    oldBook.pop_front();
                    continue;
                }
                if (priceIsBetter(newBookPriceLevel, oldBookPriceLevel)) {
                    bookElement tmp;
                    tmp.price = newBookPriceLevel;
                    tmp.qty = newBook[i - 1].qty;
                    oldBook.insert(oldBook.begin() + (i - 1), tmp);
                    emitLimit(pairId, ORDER_ACTION::ADD, oldBook[i - 1].price, oldBook[i - 1].qty);
                    continue;
                }

                if (SafeDoubleCompare(oldBookPriceLevel, newBookPriceLevel)) {
                    if (newBook.size() - 1 >= i - 1 && oldBook.size() - 1 >= i - 1) {
                        auto qtyDifference = newBook[i - 1].qty - oldBook[i - 1].qty;
                        if (qtyDifference > 0) {
                            oldBook[i - 1].qty += qtyDifference;
                            emitLimit(pairId, ORDER_ACTION::ADD, oldBook[i - 1].price, qtyDifference);
                        }
                        if (qtyDifference < 0) {
                            oldBook[i - 1].qty += qtyDifference;
                            emitLimit(pairId, ORDER_ACTION::REMOVE, oldBook[i - 1].price, -qtyDifference);
                        }
                    }

                    if (priceIsBetter(nextOldBookPriceLevel, nextNewBookPriceLevel)) {
                        emitLimit(pairId, ORDER_ACTION::REMOVE, oldBook[i].price, oldBook[i].qty);
                        oldBook.erase(oldBook.begin() + (i));
                    }
                    if (priceIsBetter(nextNewBookPriceLevel, nextOldBookPriceLevel)) {
                        ORDER_ACTION action = checkAndUpdateSeeker(nextNewBookPriceLevel);
                        bookElement tmp;
                        tmp.price = nextNewBookPriceLevel;
                        tmp.qty = newBook[i].qty;
                        oldBook.insert(oldBook.begin() + (i), tmp);
                        emitLimit(pairId, action, oldBook[i].price, oldBook[i].qty);
                    }
                }

                if (SafeDoubleCompare(nextOldBookPriceLevel, nextNewBookPriceLevel)) {
                    if (newBook.size() - 1 >= i && oldBook.size() - 1 >= i) {
                        auto qtyDifference = newBook[i].qty - oldBook[i].qty;
                        if (qtyDifference > 0) {
                            oldBook[i].qty += qtyDifference;
                            emitLimit(pairId, ORDER_ACTION::ADD, oldBook[i].price, qtyDifference);
                        }
                        if (qtyDifference < 0) {
                            oldBook[i].qty += qtyDifference;
                            emitLimit(pairId, ORDER_ACTION::REMOVE, oldBook[i].price, -qtyDifference);
                        }
                    }
                }
            } while (!SafeDoubleCompare(oldBookPriceLevel, newBookPriceLevel) ||
                     !SafeDoubleCompare(nextOldBookPriceLevel, nextNewBookPriceLevel));
        }
    }
}

void SeekerNetBoonSnapshotParserToTBT::EmitOrdersAndUpdateOldBuyBook(
    PAIR_ID pairId, std::vector<bookElement>& newBook, ORDER_TIME time
) {
    _emitOrdersAndUpdateBook(pairId, _orderBooksCache.at(pairId).oldBuySide, newBook, time, ORDER_SIDE::BUY, true);
}

void SeekerNetBoonSnapshotParserToTBT::EmitOrdersAndUpdateOldSellBook(
    PAIR_ID pairId, std::vector<bookElement>& newBook, ORDER_TIME time
) {
    _emitOrdersAndUpdateBook(pairId, _orderBooksCache.at(pairId).oldSellSide, newBook, time, ORDER_SIDE::SELL, false);
}

void SeekerNetBoonSnapshotParserToTBT::PrintFullBook(PAIR_ID pairId) {
    std::cout << std::endl;
    _printSellBook(pairId);
    std::cout << "SPREAD" << std::endl;
    _printBuyBook(pairId);
    std::cout << std::endl;
}

void SeekerNetBoonSnapshotParserToTBT::_printBuyBook(PAIR_ID pairId) {
    BookSide& book = _orderBooksCache.at(pairId).oldBuySide;
    for (auto iter = book.begin(); iter != book.end(); iter++) {
        std::cout << std::distance(book.begin(), iter) << '\t' << iter->price << '\t' << iter->qty << std::endl;
    }
    std::cout << "Level" << '\t' << "Price" << '\t' << "Qty" << std::endl;
}

void SeekerNetBoonSnapshotParserToTBT::_printSellBook(PAIR_ID pairId) {
    BookSide& book = _orderBooksCache.at(pairId).oldSellSide;
    std::cout << "Level" << '\t' << "Price" << '\t' << "Qty" << std::endl;
    for (auto iter = book.rbegin(); iter != book.rend(); iter++) {
        std::cout << std::distance(iter, book.rend()) - 1 << '\t' << iter->price << '\t' << iter->qty << std::endl;
    }
}

void SeekerNetBoonSnapshotParserToTBT::_setMinAskSeen(PAIR_ID pairId, double newValue) {
    _seekerBoundCache.at(pairId).minAskSeen = newValue;
}

void SeekerNetBoonSnapshotParserToTBT::_setMaxBidSeen(PAIR_ID pairId, double newValue) {
    _seekerBoundCache.at(pairId).maxBidSeen = newValue;
}

void SeekerNetBoonSnapshotParserToTBT::_emitOrder(PAIR_ID pairId, const Order& order) {
    _emittedOrders.push_back(order);
}

} // namespace data_feed_parser
} // namespace data_feed
} // namespace cl
