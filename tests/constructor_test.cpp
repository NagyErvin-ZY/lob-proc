#include "test_common.h"

TEST(ConstructorTest, InitializesEmptyBooksForSinglePair) {
    SeekerNetBoonSnapshotParserToTBT parser({1});
    EXPECT_TRUE(parser.getBuySide(1).empty());
    EXPECT_TRUE(parser.getSellSide(1).empty());
}

TEST(ConstructorTest, InitializesEmptyBooksForMultiplePairs) {
    SeekerNetBoonSnapshotParserToTBT parser({1, 2, 3});
    for (PAIR_ID id : {1, 2, 3}) {
        EXPECT_TRUE(parser.getBuySide(id).empty());
        EXPECT_TRUE(parser.getSellSide(id).empty());
    }
}

TEST(ConstructorTest, InitializesSeekerBoundsCorrectly) {
    SeekerNetBoonSnapshotParserToTBT parser({1});
    auto bounds = parser.getSeekerBounds(1);
    EXPECT_DOUBLE_EQ(bounds.maxBidSeen, -MAX_DOUBLE);
    EXPECT_DOUBLE_EQ(bounds.minAskSeen, MAX_DOUBLE);
}

TEST(ConstructorTest, EmptyPairListCreatesNoBooks) {
    SeekerNetBoonSnapshotParserToTBT parser({});
    EXPECT_THROW(parser.getBuySide(1), std::out_of_range);
}

TEST(ConstructorTest, DuplicatePairIdsHandled) {
    SeekerNetBoonSnapshotParserToTBT parser({1, 1, 1});
    EXPECT_TRUE(parser.getBuySide(1).empty());
}
