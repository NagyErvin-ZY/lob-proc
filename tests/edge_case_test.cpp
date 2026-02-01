#include "test_common.h"
#include <limits>

class EdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(EdgeCaseTest, ZeroPriceLevel) {
    std::vector<bookElement> newBook = {makeBookElement(0.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_DOUBLE_EQ(parser->getBuySide(1)[0].price, 0.0);
}

TEST_F(EdgeCaseTest, ZeroQuantity) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, 0)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
}

TEST_F(EdgeCaseTest, MaxTimeValue) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, std::numeric_limits<ORDER_TIME>::max());

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
}

TEST_F(EdgeCaseTest, LargeBookWithManyLevels) {
    std::vector<bookElement> newBook;
    for (int i = 0; i < 100; i++) {
        newBook.push_back(makeBookElement(100.0 - i, 50 + i));
    }
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 100);
}

TEST_F(EdgeCaseTest, RapidBookUpdates) {
    for (int i = 0; i < 100; i++) {
        std::vector<bookElement> newBook = {makeBookElement(100.0 + i, 50)};
        parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000 + i);
    }

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
}

TEST_F(EdgeCaseTest, AlternatingAddRemove) {
    for (int i = 0; i < 10; i++) {
        std::vector<bookElement> addBook = {makeBookElement(100.0, 50)};
        parser->EmitOrdersAndUpdateOldBuyBook(1, addBook, 1000 + i * 2);

        std::vector<bookElement> emptyBook;
        parser->EmitOrdersAndUpdateOldBuyBook(1, emptyBook, 1000 + i * 2 + 1);
    }

    EXPECT_TRUE(parser->getBuySide(1).empty());
}

TEST_F(EdgeCaseTest, PriceAtEpsilonBoundary) {
    std::vector<bookElement> book1 = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 25, 100.0 + DoubleComparisonEpsilon / 2, 2000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_EQ(parser->getBuySide(1)[0].qty, 25);
}

TEST_F(EdgeCaseTest, VerySmallQuantityDifference) {
    std::vector<bookElement> book1 = {makeBookElement(100.0, 100)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);

    std::vector<bookElement> book2 = {makeBookElement(100.0, 99)};
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldBuyBook(1, book2, 2000);

    EXPECT_EQ(parser->getBuySide(1)[0].qty, 99);
}

TEST_F(EdgeCaseTest, PriceNearDoubleMax) {
    std::vector<bookElement> newBook = {makeBookElement(MAX_DOUBLE / 2, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
}
