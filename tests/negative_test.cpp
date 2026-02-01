#include "test_common.h"

class NegativeTests : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(NegativeTests, AccessNonExistentPairThrows) {
    EXPECT_THROW(parser->getBuySide(999), std::out_of_range);
    EXPECT_THROW(parser->getSellSide(999), std::out_of_range);
    EXPECT_THROW(parser->getSeekerBounds(999), std::out_of_range);
}

TEST_F(NegativeTests, UpdateNonExistentPairThrows) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    EXPECT_THROW(parser->EmitOrdersAndUpdateOldBuyBook(999, newBook, 1000), std::out_of_range);
    EXPECT_THROW(parser->EmitOrdersAndUpdateOldSellBook(999, newBook, 1000), std::out_of_range);
}

TEST_F(NegativeTests, MarketOrderOnNonExistentPairThrows) {
    EXPECT_THROW(parser->EmitMarketOrderAndUpdateBuyBook(999, 100, 50.0, 1000), std::out_of_range);
    EXPECT_THROW(parser->EmitMarketOrderAndUpdateSellBook(999, 100, 50.0, 1000), std::out_of_range);
}

TEST_F(NegativeTests, NegativeQuantityBehavior) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, -50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    auto& book = parser->getBuySide(1);
    EXPECT_EQ(book.size(), 1);
}

TEST_F(NegativeTests, NegativePriceBehavior) {
    std::vector<bookElement> newBook = {makeBookElement(-100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    EXPECT_EQ(parser->getBuySide(1).size(), 1);
}

TEST_F(NegativeTests, PrintFullBookNonExistentPairThrows) {
    EXPECT_THROW(parser->PrintFullBook(999), std::out_of_range);
}
