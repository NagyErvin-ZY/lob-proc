#include "test_common.h"

class SellBookUpdateTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1, 2});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(SellBookUpdateTest, AddOrdersToEmptyBook) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, 50),
        makeBookElement(101.0, 30)
    };
    parser->EmitOrdersAndUpdateOldSellBook(1, newBook, 1000);

    auto& book = parser->getSellSide(1);
    ASSERT_EQ(book.size(), 2);
    EXPECT_DOUBLE_EQ(book[0].price, 100.0);
    EXPECT_EQ(book[0].qty, 50);
}

TEST_F(SellBookUpdateTest, ClearAllOrdersFromSellBook) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, 50),
        makeBookElement(101.0, 30)
    };
    parser->EmitOrdersAndUpdateOldSellBook(1, newBook, 1000);

    parser->clearEmittedOrders();
    std::vector<bookElement> emptyBook;
    parser->EmitOrdersAndUpdateOldSellBook(1, emptyBook, 2000);

    EXPECT_TRUE(parser->getSellSide(1).empty());

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);
    for (auto& order : emitted) {
        EXPECT_EQ(order.action, ORDER_ACTION::REMOVE);
    }
}

TEST_F(SellBookUpdateTest, BothBooksEmptyNoAction) {
    parser->clearEmittedOrders();
    std::vector<bookElement> emptyBook;
    parser->EmitOrdersAndUpdateOldSellBook(1, emptyBook, 1000);

    EXPECT_TRUE(parser->getSellSide(1).empty());
    EXPECT_TRUE(parser->getEmittedOrders().empty());
}

TEST_F(SellBookUpdateTest, MinAskUpdatesOnNewLowPrice) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, 50),
        makeBookElement(90.0, 30)
    };
    parser->EmitOrdersAndUpdateOldSellBook(1, newBook, 1000);

    auto bounds = parser->getSeekerBounds(1);
    EXPECT_LT(bounds.minAskSeen, MAX_DOUBLE);
}

TEST_F(SellBookUpdateTest, QuantityIncreaseAtSamePrice) {
    std::vector<bookElement> book1 = {
        makeBookElement(100.0, 50),
        makeBookElement(101.0, 30)
    };
    parser->EmitOrdersAndUpdateOldSellBook(1, book1, 1000);

    std::vector<bookElement> book2 = {
        makeBookElement(100.0, 80),
        makeBookElement(101.0, 30)
    };
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldSellBook(1, book2, 2000);

    auto& book = parser->getSellSide(1);
    EXPECT_EQ(book[0].qty, 80);
}

TEST_F(SellBookUpdateTest, QuantityDecreaseAtSamePrice) {
    std::vector<bookElement> book1 = {
        makeBookElement(100.0, 50),
        makeBookElement(101.0, 30)
    };
    parser->EmitOrdersAndUpdateOldSellBook(1, book1, 1000);

    std::vector<bookElement> book2 = {
        makeBookElement(100.0, 20),
        makeBookElement(101.0, 30)
    };
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldSellBook(1, book2, 2000);

    auto& book = parser->getSellSide(1);
    EXPECT_EQ(book[0].qty, 20);
}
