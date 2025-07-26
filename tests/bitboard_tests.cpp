#include <gtest/gtest.h>
#include "bitboard.h"

TEST(BoardTest, RookAttacksMatchesRayForAllSquares) {
    for (int sq = 0; sq < 64; ++sq) {
        bitboard_t expected = bb::ray<NORTH, SOUTH, EAST, WEST>(static_cast<square_t>(sq));
        bitboard_t actual   = bb::attacks<ROOK>(static_cast<square_t>(sq));
        EXPECT_EQ(actual, expected) << "Mismatch at square: " << sq;
    }
}

TEST(BoardTest, BishopAttacksMatchesRayForAllSquares) {
    for (int sq = 0; sq < 64; ++sq) {
        bitboard_t expected = bb::ray<NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST>(static_cast<square_t>(sq));
        bitboard_t actual   = bb::attacks<BISHOP>(static_cast<square_t>(sq));
        EXPECT_EQ(actual, expected) << "Mismatch at square: " << sq;
    }
}