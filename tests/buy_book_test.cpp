#include "test_common.h"

class BuyBookUpdateTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1, 2});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(BuyBookUpdateTest, AddOrdersToEmptyBook) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, 50),
        makeBookElement(99.0, 30)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    auto& book = parser->getBuySide(1);
    ASSERT_EQ(book.size(), 2);
    EXPECT_DOUBLE_EQ(book[0].price, 100.0);
    EXPECT_EQ(book[0].qty, 50);
    EXPECT_DOUBLE_EQ(book[1].price, 99.0);
    EXPECT_EQ(book[1].qty, 30);
}

TEST_F(BuyBookUpdateTest, AddOrdersEmitsCorrectActions) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, 50),
        makeBookElement(99.0, 30)
    };
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);
    EXPECT_EQ(emitted[0].action, ORDER_ACTION::SEEKER_ADD);
    EXPECT_EQ(emitted[0].side, ORDER_SIDE::BUY);
    EXPECT_EQ(emitted[0].type, ORDER_TYPE::LIMIT);
}

TEST_F(BuyBookUpdateTest, ClearAllOrdersFromBook) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, 50),
        makeBookElement(99.0, 30)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);
    ASSERT_EQ(parser->getBuySide(1).size(), 2);

    parser->clearEmittedOrders();
    std::vector<bookElement> emptyBook;
    parser->EmitOrdersAndUpdateOldBuyBook(1, emptyBook, 2000);

    EXPECT_TRUE(parser->getBuySide(1).empty());

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);
    for (auto& order : emitted) {
        EXPECT_EQ(order.action, ORDER_ACTION::REMOVE);
        EXPECT_EQ(order.type, ORDER_TYPE::LIMIT);
    }
}

TEST_F(BuyBookUpdateTest, BothBooksEmptyNoAction) {
    parser->clearEmittedOrders();
    std::vector<bookElement> emptyBook;
    parser->EmitOrdersAndUpdateOldBuyBook(1, emptyBook, 1000);

    EXPECT_TRUE(parser->getBuySide(1).empty());
    EXPECT_TRUE(parser->getEmittedOrders().empty());
}

TEST_F(BuyBookUpdateTest, QuantityIncreaseAtSamePrice) {
    std::vector<bookElement> book1 = {
        makeBookElement(100.0, 50),
        makeBookElement(99.0, 30)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);

    std::vector<bookElement> book2 = {
        makeBookElement(100.0, 80),
        makeBookElement(99.0, 30)
    };
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldBuyBook(1, book2, 2000);

    auto& book = parser->getBuySide(1);
    EXPECT_EQ(book[0].qty, 80);
}

TEST_F(BuyBookUpdateTest, QuantityDecreaseAtSamePrice) {
    std::vector<bookElement> book1 = {
        makeBookElement(100.0, 50),
        makeBookElement(99.0, 30)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);

    std::vector<bookElement> book2 = {
        makeBookElement(100.0, 20),
        makeBookElement(99.0, 30)
    };
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldBuyBook(1, book2, 2000);

    auto& book = parser->getBuySide(1);
    EXPECT_EQ(book[0].qty, 20);
}

TEST_F(BuyBookUpdateTest, SingleElementBook) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    ASSERT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_DOUBLE_EQ(parser->getBuySide(1)[0].price, 100.0);
}

TEST_F(BuyBookUpdateTest, LargeQuantityValues) {
    std::vector<bookElement> newBook = {
        makeBookElement(100.0, INT_MAX - 1),
        makeBookElement(99.0, INT_MAX / 2)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    auto& book = parser->getBuySide(1);
    EXPECT_EQ(book[0].qty, INT_MAX - 1);
}

TEST_F(BuyBookUpdateTest, VerySmallPriceValues) {
    std::vector<bookElement> newBook = {
        makeBookElement(0.00001, 50),
        makeBookElement(0.000001, 30)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    ASSERT_EQ(parser->getBuySide(1).size(), 2);
}

TEST_F(BuyBookUpdateTest, VeryLargePriceValues) {
    std::vector<bookElement> newBook = {
        makeBookElement(1e10, 50),
        makeBookElement(1e9, 30)
    };
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    ASSERT_EQ(parser->getBuySide(1).size(), 2);
}
