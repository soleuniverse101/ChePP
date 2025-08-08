//
// Created by paul on 7/30/25.
//

#include <ChePP/engine/movegen.h>
#include <gtest/gtest.h>


TEST(ZobristTranspositions, EnPassantRightsAffectHash) {
    bb::init();
    zobrist_t::init(10);

    position_t pos1, pos2;
    pos1.from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    pos2.from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");

    EXPECT_NE(pos1.hash(), pos2.hash());

    pos1.from_fen("k1K5/3p4/8/4P3/8/8/8/8 b - - 0 1");
    pos2.from_fen("k1K5/8/8/3pP3/8/8/8/8 w - - 0 1");

    pos1.do_move(move_t::make<NORMAL>(D7, D5));

    EXPECT_NE(pos1.hash(), pos2.hash());

    pos1.do_move(move_t::make<EN_PASSANT>(E5, D6));
    pos2.do_move(move_t::make<EN_PASSANT>(E5, D6));

    EXPECT_EQ(pos1.hash(), pos2.hash());
}

TEST(ZobristTranspositions, CastlingRightsAffectHash) {
    bb::init();
    zobrist_t::init(10);

    position_t pos1, pos2;
    pos1.from_fen("rnbqkbnr/8/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    pos2.from_fen("rnbqkbnr/8/8/8/8/8/PPPPPPPP/RNBQKBNR b KQ - 0 1");

    EXPECT_NE(pos1.hash(), pos2.hash());

    pos1.do_move(move_t::make<NORMAL>(E8, E7));
    pos1.do_move(move_t::null());
    pos1.do_move(move_t::make<NORMAL>(E7, E8));
    pos1.do_move(move_t::null());


    EXPECT_EQ(pos1.hash(), pos2.hash());
}


