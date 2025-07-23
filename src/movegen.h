#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include "bitboard.h"
#include "magics.h"
#include "position.h"
#include <system_error>

typedef enum movegen_type_t : int8_t
{
    QUIET,
    CAPTURE,
    ESCAPE,
    PSEUDO_LEGAL,
    LEGAL
} movegen_type_t;

class move_t
{
  public:
    move_t(square_t from, square_t to);
    square_t from();
    square_t to();
};

class move_list_t
{
  public:
    static const size_t max_moves = 256;
    void                add(move_t m);
};

template <movegen_type_t mt, color_t c>
void gen_pawn_moves(position_t* pos, state_t* state, move_list_t* list)
{
    constexpr direction_t up               = c == WHITE ? NORTH : SOUTH;
    constexpr bitboard_t  bb_last_rank     = c == WHITE ? bb::rk_mask(RANK_8) : bb::rk_mask(RANK_1);
    constexpr bitboard_t bb_promotion_rank = c == WHITE ? bb::rk_mask(RANK_7) : bb::rk_mask(RANK_2);
    constexpr bitboard_t bb_third_rank     = c == WHITE ? bb::rk_mask(RANK_3) : bb::rk_mask(RANK_5);
    const bitboard_t     available         = ~pos->global_occupancy;
    const bitboard_t     enemy             = pos->color_occupancy[~c];
    const bitboard_t     pawns             = pos->pieces_bb(c, PAWN);
    const bitboard_t     ep_bb =
        (state->ep_square == NO_SQUARE ? bb::empty : bb::sq_mask(state->ep_square));
    // straight one
    if constexpr (mt != CAPTURE)
    {
        bitboard_t single_push = bb::shift<up>(pawns & ~bb_promotion_rank) & available;
        bitboard_t double_push = bb::shift<up>(single_push & bb_third_rank) & available;
        while (single_push)
        {
            square_t sq = square_t(pop_lsb(single_push));
            list->add(move_t(sq - up, sq));
        }
        while (double_push)
        {
            square_t sq = square_t(pop_lsb(double_push));
            list->add(move_t(sq - 2 * up, sq));
        }
    }
    bitboard_t promotion = pawns & bb_promotion_rank;
    if (promotion)
    {
    }
    if constexpr (mt & CAPTURE)
    {
        bitboard_t remaining  = pawns;
        bitboard_t capturable = enemy | ep_bb;
        while (remaining)
        {
            square_t   from    = square_t(pop_lsb(remaining));
            bitboard_t attacks = (bb::attacks<PAWN>(from, enemy, c)) & capturable;
            while (attacks)
            {
                square_t to = square_t(pop_lsb(attacks));
                list->add(move_t(from, to));
            }
        }
    }
}
#endif
