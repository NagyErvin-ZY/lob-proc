#include "test_common.h"

class MarketOrderBuyTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(MarketOrderBuyTest, MarketOrderOnEmptyBookEmitsIceberg) {
    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 100, 50.0, 1000);

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);

    EXPECT_EQ(emitted[0].type, ORDER_TYPE::ICEBERG);
    EXPECT_EQ(emitted[0].side, ORDER_SIDE::BUY);
    EXPECT_EQ(emitted[0].qty, 100);

    EXPECT_EQ(emitted[1].type, ORDER_TYPE::MARKET);
    EXPECT_EQ(emitted[1].side, ORDER_SIDE::SELL);
}

TEST_F(MarketOrderBuyTest, MarketOrderPartialFill) {
    std::vector<bookElement> buyBook = {makeBookElement(50.0, 200)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 50, 50.0, 2000);

    auto& book = parser->getBuySide(1);
    ASSERT_EQ(book.size(), 1);
    EXPECT_EQ(book[0].qty, 150);

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 1);
    EXPECT_EQ(emitted[0].type, ORDER_TYPE::MARKET);
}

TEST_F(MarketOrderBuyTest, MarketOrderExactFill) {
    std::vector<bookElement> buyBook = {makeBookElement(50.0, 100)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 100, 50.0, 2000);

    EXPECT_TRUE(parser->getBuySide(1).empty());

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 1);
    EXPECT_EQ(emitted[0].type, ORDER_TYPE::MARKET);
}

TEST_F(MarketOrderBuyTest, MarketOrderOverfillDetectsIceberg) {
    std::vector<bookElement> buyBook = {makeBookElement(50.0, 40)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 100, 50.0, 2000);

    EXPECT_TRUE(parser->getBuySide(1).empty());

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);

    EXPECT_EQ(emitted[0].type, ORDER_TYPE::ICEBERG);
    EXPECT_EQ(emitted[0].qty, 60);

    EXPECT_EQ(emitted[1].type, ORDER_TYPE::MARKET);
}

TEST_F(MarketOrderBuyTest, MarketOrderAtWrongPriceLogsError) {
    std::vector<bookElement> buyBook = {makeBookElement(50.0, 100)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 50, 60.0, 2000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_TRUE(parser->getEmittedOrders().empty());
}

TEST_F(MarketOrderBuyTest, MarketOrderZeroQuantity) {
    std::vector<bookElement> buyBook = {makeBookElement(50.0, 100)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateBuyBook(1, 0, 50.0, 2000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
}

// Sell side market order tests

class MarketOrderSellTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(MarketOrderSellTest, MarketOrderOnEmptyBookEmitsIceberg) {
    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateSellBook(1, 100, 50.0, 1000);

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);

    EXPECT_EQ(emitted[0].type, ORDER_TYPE::ICEBERG);
    EXPECT_EQ(emitted[0].side, ORDER_SIDE::SELL);

    EXPECT_EQ(emitted[1].type, ORDER_TYPE::MARKET);
    EXPECT_EQ(emitted[1].side, ORDER_SIDE::BUY);
}

TEST_F(MarketOrderSellTest, MarketOrderPartialFillSellSide) {
    std::vector<bookElement> sellBook = {makeBookElement(50.0, 200)};
    parser->EmitOrdersAndUpdateOldSellBook(1, sellBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateSellBook(1, 50, 50.0, 2000);

    auto& book = parser->getSellSide(1);
    ASSERT_EQ(book.size(), 1);
    EXPECT_EQ(book[0].qty, 150);
}

TEST_F(MarketOrderSellTest, MarketOrderExactFillSellSide) {
    std::vector<bookElement> sellBook = {makeBookElement(50.0, 100)};
    parser->EmitOrdersAndUpdateOldSellBook(1, sellBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateSellBook(1, 100, 50.0, 2000);

    EXPECT_TRUE(parser->getSellSide(1).empty());
}

TEST_F(MarketOrderSellTest, MarketOrderOverfillSellSide) {
    std::vector<bookElement> sellBook = {makeBookElement(50.0, 40)};
    parser->EmitOrdersAndUpdateOldSellBook(1, sellBook, 1000);

    parser->clearEmittedOrders();
    parser->EmitMarketOrderAndUpdateSellBook(1, 100, 50.0, 2000);

    EXPECT_TRUE(parser->getSellSide(1).empty());

    auto& emitted = parser->getEmittedOrders();
    ASSERT_EQ(emitted.size(), 2);
    EXPECT_EQ(emitted[0].type, ORDER_TYPE::ICEBERG);
    EXPECT_EQ(emitted[0].qty, 60);
}
