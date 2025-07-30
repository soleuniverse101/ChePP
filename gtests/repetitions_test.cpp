//
// Created by paul on 7/30/25.
//

#include <ChePP/movegen.h>
#include <gtest/gtest.h>

TEST(ThreeFoldRepetitions, FourNullMoveIsDraw)
{
    bb::init();
    zobrist_t::init(10);

    position_t pos;
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    for (int i = 0; i < 4; i++)
    {
        ASSERT_EQ(pos.is_draw(), false);
        pos.do_move(move_t::null());

    }
    ASSERT_EQ(pos.is_draw() , true);
}

TEST(ThreeFoldRepetitions, TwoNightShuffleIsDraw)
{
    bb::init();
    zobrist_t::init(10);

    position_t pos;
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos.do_move(move_t::make<NORMAL>(G1, H3));
    pos.do_move(move_t::make<NORMAL>(G8, H6));

    for (int i = 0; i < 2; i++)
    {
        ASSERT_EQ(pos.is_draw(), false) << " at iterations nb: " << i;
        pos.do_move(move_t::make<NORMAL>(B1, C3));
        pos.do_move(move_t::make<NORMAL>(B8, C6));
        pos.do_move(move_t::make<NORMAL>(C3, B1));
        pos.do_move(move_t::make<NORMAL>(C6, B8));
    }
    ASSERT_EQ(pos.is_draw() , true);
}

TEST(FiftyMoveRule, FiftyRookShuffleIsDraw)
{
    bb::init();
    zobrist_t::init(10);

    position_t pos;
    pos.from_fen("K1k5/pppppppp/8/8/8/8/8/R7 w KQkq - 0 1");

    square_t sq_from = A1;
    for (square_t sq_to = B1; sq_to < static_cast<square_t>(B1 + 25); sq_to = static_cast<square_t>(sq_to + 1))
    {
        const move_t mv = move_t::make<NORMAL>(sq_from, sq_to);
        pos.do_move(mv);
        pos.do_move(move_t::null());
        pos.do_move(move_t::null());
        pos.do_move(move_t::null());

        sq_from = sq_to;

        ASSERT_EQ(pos.is_draw() , sq_to == B1 + 24) << "halfmoves clock: " << pos.halfmove_clock();
    }

}

TEST(FiftyMoveRule, PawnMoveResetsFiftyRookShuffleIsDraw)
{
    bb::init();
    zobrist_t::init(10);

    position_t pos;
    pos.from_fen("K1k5/pppppppp/8/8/8/8/8/R7 w KQkq - 0 1");

    square_t sq_from = A1;
    for (square_t sq_to = B1; sq_to < static_cast<square_t>(B1 + 25); sq_to = static_cast<square_t>(sq_to + 1))
    {
        const move_t mv = move_t::make<NORMAL>(sq_from, sq_to);
        if (sq_from == A1 + 10)
        {
            pos.do_move(move_t::null());
            pos.do_move(move_t::make<NORMAL>(H7, H6));
        }
        pos.do_move(mv);
        pos.do_move(move_t::null());
        pos.do_move(move_t::null());
        pos.do_move(move_t::null());

        sq_from = sq_to;

        ASSERT_EQ(pos.is_draw(), false) << "halfmoves clock: " << pos.halfmove_clock();
    }

}