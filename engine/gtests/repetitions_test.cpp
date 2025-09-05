//
// Created by paul on 7/30/25.
//

#include <ChePP/engine/movegen.h>
#include <gtest/gtest.h>

TEST(ThreeFoldRepetitions, FourNullMoveIsDraw)
{
    Position pos;
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    for (int i = 0; i < 4; i++)
    {
        ASSERT_EQ(pos.is_draw(), false);
        pos.do_move(Move::null());

    }
    ASSERT_EQ(pos.is_draw() , true);
}

TEST(ThreeFoldRepetitions, TwoNightShuffleIsDraw)
{
    Position pos;
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos.do_move(Move::make<NORMAL>(G1, H3));
    pos.do_move(Move::make<NORMAL>(G8, H6));

    for (int i = 0; i < 2; i++)
    {
        ASSERT_EQ(pos.is_draw(), false) << " at iterations nb: " << i;
        pos.do_move(Move::make<NORMAL>(B1, C3));
        pos.do_move(Move::make<NORMAL>(B8, C6));
        pos.do_move(Move::make<NORMAL>(C3, B1));
        pos.do_move(Move::make<NORMAL>(C6, B8));
    }
    ASSERT_EQ(pos.is_draw() , true);
}

TEST(FiftyMoveRule, FiftyRookShuffleIsDraw)
{
    Position pos;
    pos.from_fen("K1k5/pppppppp/8/8/8/8/8/R7 w KQkq - 0 1");

    Square sq_from = A1;
    for (Square sq_to = B1; sq_to < static_cast<Square>(B1 + 25); sq_to = static_cast<Square>(sq_to + 1))
    {
        const Move mv = Move::make<NORMAL>(sq_from, sq_to);
        pos.do_move(mv);
        pos.do_move(Move::null());
        pos.do_move(Move::null());
        pos.do_move(Move::null());

        sq_from = sq_to;

        ASSERT_EQ(pos.is_draw() , sq_to == B1 + 24) << "halfmoves clock: " << pos.halfmove_clock();
    }

}

TEST(FiftyMoveRule, PawnMoveResetsFiftyRookShuffleIsDraw)
{
    Position pos;
    pos.from_fen("K1k5/pppppppp/8/8/8/8/8/R7 w KQkq - 0 1");

    Square sq_from = A1;
    for (Square sq_to = B1; sq_to < static_cast<Square>(B1 + 25); sq_to = static_cast<Square>(sq_to + 1))
    {
        const Move mv = Move::make<NORMAL>(sq_from, sq_to);
        if (sq_from == A1 + 10)
        {
            pos.do_move(Move::null());
            pos.do_move(Move::make<NORMAL>(H7, H6));
        }
        pos.do_move(mv);
        pos.do_move(Move::null());
        pos.do_move(Move::null());
        pos.do_move(Move::null());

        sq_from = sq_to;

        ASSERT_EQ(pos.is_draw(), false) << "halfmoves clock: " << pos.halfmove_clock();
    }

}