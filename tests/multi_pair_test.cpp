#include "test_common.h"

class MultiPairTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1, 2, 3});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(MultiPairTest, PairBooksAreIsolated) {
    std::vector<bookElement> book1 = {makeBookElement(100.0, 50)};
    std::vector<bookElement> book2 = {makeBookElement(200.0, 100)};

    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);
    parser->EmitOrdersAndUpdateOldBuyBook(2, book2, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_EQ(parser->getBuySide(2).size(), 1);
    EXPECT_DOUBLE_EQ(parser->getBuySide(1)[0].price, 100.0);
    EXPECT_DOUBLE_EQ(parser->getBuySide(2)[0].price, 200.0);
    EXPECT_TRUE(parser->getBuySide(3).empty());
}

TEST_F(MultiPairTest, SeekerBoundsIsolatedPerPair) {
    std::vector<bookElement> book1 = {makeBookElement(100.0, 50)};
    std::vector<bookElement> book2 = {makeBookElement(500.0, 100)};

    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);
    parser->EmitOrdersAndUpdateOldBuyBook(2, book2, 1000);

    auto bounds1 = parser->getSeekerBounds(1);
    auto bounds2 = parser->getSeekerBounds(2);

    EXPECT_NE(bounds1.maxBidSeen, bounds2.maxBidSeen);
}

TEST_F(MultiPairTest, MarketOrdersAffectOnlyTargetPair) {
    std::vector<bookElement> book1 = {makeBookElement(100.0, 50)};
    std::vector<bookElement> book2 = {makeBookElement(100.0, 50)};

    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);
    parser->EmitOrdersAndUpdateOldBuyBook(2, book2, 1000);

    parser->EmitMarketOrderAndUpdateBuyBook(1, 25, 100.0, 2000);

    EXPECT_EQ(parser->getBuySide(1)[0].qty, 25);
    EXPECT_EQ(parser->getBuySide(2)[0].qty, 50);
}

TEST_F(MultiPairTest, IndependentBuySellSidesPerPair) {
    std::vector<bookElement> buyBook = {makeBookElement(100.0, 50)};
    std::vector<bookElement> sellBook = {makeBookElement(101.0, 30)};

    parser->EmitOrdersAndUpdateOldBuyBook(1, buyBook, 1000);
    parser->EmitOrdersAndUpdateOldSellBook(1, sellBook, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_EQ(parser->getSellSide(1).size(), 1);
    EXPECT_DOUBLE_EQ(parser->getBuySide(1)[0].price, 100.0);
    EXPECT_DOUBLE_EQ(parser->getSellSide(1)[0].price, 101.0);
}

TEST_F(MultiPairTest, ClearOnePairDoesNotAffectOthers) {
    std::vector<bookElement> book = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, book, 1000);
    parser->EmitOrdersAndUpdateOldBuyBook(2, book, 1000);
    parser->EmitOrdersAndUpdateOldBuyBook(3, book, 1000);

    std::vector<bookElement> emptyBook;
    parser->EmitOrdersAndUpdateOldBuyBook(2, emptyBook, 2000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
    EXPECT_TRUE(parser->getBuySide(2).empty());
    EXPECT_EQ(parser->getBuySide(3).size(), 1);
}
