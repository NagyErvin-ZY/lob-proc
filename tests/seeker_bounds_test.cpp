#include "test_common.h"

class SeekerBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<SeekerNetBoonSnapshotParserToTBT>(std::vector<PAIR_ID>{1});
    }
    std::unique_ptr<SeekerNetBoonSnapshotParserToTBT> parser;
};

TEST_F(SeekerBoundsTest, MaxBidUpdatesOnBuySideHigherPrice) {
    auto initialBounds = parser->getSeekerBounds(1);
    EXPECT_DOUBLE_EQ(initialBounds.maxBidSeen, -MAX_DOUBLE);

    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    auto newBounds = parser->getSeekerBounds(1);
    EXPECT_GT(newBounds.maxBidSeen, -MAX_DOUBLE);
}

TEST_F(SeekerBoundsTest, MinAskUpdatesOnSellSideLowerPrice) {
    auto initialBounds = parser->getSeekerBounds(1);
    EXPECT_DOUBLE_EQ(initialBounds.minAskSeen, MAX_DOUBLE);

    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldSellBook(1, newBook, 1000);

    auto newBounds = parser->getSeekerBounds(1);
    EXPECT_LT(newBounds.minAskSeen, MAX_DOUBLE);
}

TEST_F(SeekerBoundsTest, MultipleUpdatesTrackExtremes) {
    std::vector<bookElement> book1 = {makeBookElement(100.0, 50)};
    parser->EmitOrdersAndUpdateOldBuyBook(1, book1, 1000);

    auto bounds1 = parser->getSeekerBounds(1);
    double maxBid1 = bounds1.maxBidSeen;

    std::vector<bookElement> book2 = {
        makeBookElement(120.0, 50),
        makeBookElement(100.0, 30)
    };
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldBuyBook(1, book2, 2000);

    auto bounds2 = parser->getSeekerBounds(1);
    EXPECT_GE(bounds2.maxBidSeen, maxBid1);
}

TEST_F(SeekerBoundsTest, SeekerAddActionEmittedForNewHigherBuyPrice) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldBuyBook(1, newBook, 1000);

    auto& emitted = parser->getEmittedOrders();
    ASSERT_GE(emitted.size(), 1);
    EXPECT_EQ(emitted[0].action, ORDER_ACTION::SEEKER_ADD);
}

TEST_F(SeekerBoundsTest, SeekerAddActionEmittedForNewLowerSellPrice) {
    std::vector<bookElement> newBook = {makeBookElement(100.0, 50)};
    parser->clearEmittedOrders();
    parser->EmitOrdersAndUpdateOldSellBook(1, newBook, 1000);

    auto& emitted = parser->getEmittedOrders();
    ASSERT_GE(emitted.size(), 1);
    EXPECT_EQ(emitted[0].action, ORDER_ACTION::SEEKER_ADD);
}
