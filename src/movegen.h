#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include "bitboard.h"
#include "move.h"
#include "position.h"

#include <memory>

enum movegen_type_t : int8_t
{
    QUIET,
    CAPTURE,
    ESCAPE,
    PSEUDO_LEGAL,
    LEGAL
} ;






class move_list_t
{
  public:
    static constexpr size_t max_moves = 256;
    void                add(move_t m);
};


template <color_t c>
void gen_pawn_moves(const position_t& pos, move_list_t* list, bitboard_t check_mask = bb::full)
{
    //check mask refers to king attackers u rays of king attackers
    constexpr direction_t up               = c == WHITE ? NORTH : SOUTH;
    constexpr direction_t right             = c == WHITE ? EAST : WEST;
    constexpr bitboard_t bb_promotion_rank = c == WHITE ? bb::rk_mask(RANK_7) : bb::rk_mask(RANK_2);
    constexpr bitboard_t bb_third_rank     = c == WHITE ? bb::rk_mask(RANK_3) : bb::rk_mask(RANK_5);
    const bitboard_t     available         = ~pos.color_occupancy(ANY);
    const bitboard_t     enemy             = pos.color_occupancy(~c);
    const bitboard_t     pawns             = pos.pieces_bb(c, PAWN);
    const bitboard_t     ep_bb =
        (pos.ep_square() == NO_SQUARE ? bb::empty : bb::sq_mask(pos.ep_square()));
    // straight
    {
        bitboard_t single_push = bb::shift<up>(pawns & ~bb_promotion_rank) & available;
        bitboard_t double_push = bb::shift<up>(single_push & bb_third_rank) & available & check_mask;
        single_push &= check_mask;
        while (single_push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(single_push));
            list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - up), sq));
        }
        while (double_push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(double_push));
            list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - 2 * up), sq));
        }
    }
    // promotion
    if (const bitboard_t promotions = pawns & bb_promotion_rank)
    {
        bitboard_t push = bb::shift<up>(promotions) & available & check_mask;
        while (push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(push));
            list->add(move_t::make<PROMOTION>(static_cast<square_t>(sq - up), sq));
        }
        bitboard_t  take_right = bb::shift<up + right>(promotions) & enemy & check_mask;
        while (take_right)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_right));
            list->add(move_t::make<PROMOTION>(static_cast<square_t>(sq - (up  + right)), sq));
        }
        bitboard_t  take_left = bb::shift<up - right>(promotions) & enemy & check_mask;
        while (take_left)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_left));
            list->add(move_t::make<PROMOTION>(static_cast<square_t>(sq - (up  - right)), sq));
        }
    }
    //capture
    {
        bitboard_t capturable = enemy | ep_bb;
        bitboard_t  take_right = bb::shift<up + right>(pawns & ~bb_promotion_rank) & capturable & check_mask;
        while (take_right)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(take_right)); sq == pos.ep_square()) [[unlikely]]
            {
                list->add(move_t::make<EN_PASSANT>(static_cast<square_t>(sq - (up  + right)), sq));

            } else [[likely]]
            {
                list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - (up  + right)), sq));

            }
        }
        bitboard_t  take_left = bb::shift<up - right>(pawns & ~bb_promotion_rank) & capturable & check_mask;
        while (take_left)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(take_left)); sq == pos.ep_square()) [[unlikely]]
            {
                list->add(move_t::make<EN_PASSANT>(static_cast<square_t>(sq - (up  - right)), sq));

            } else [[likely]]
            {
                list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - (up  - right)), sq));

            }
        }
    }
}


template <piece_type_t pc, color_t c>
void gen_pc_moves(const position_t& pos, move_list_t& list, bitboard_t check_mask = bb::full)
{
    bitboard_t bb = pos.pieces_occupancy(c, pc);
    while (bb)
    {
        auto from = static_cast<square_t>(pop_lsb(bb));
        bitboard_t attacks = bb::attacks<pc>(from, pos.color_occupancy(ANY)) & ~pos.color_occupancy(c) & check_mask;
        while (attacks)
        {
            list.add(move_t::make<NORMAL>(from,static_cast<square_t>(pop_lsb(attacks))));
        }
    }
}


template <color_t c>
void gen_castling(const position_t& pos, move_list_t& list, const bitboard_t check_mask = bb::full)
{
    using cr = castling_rights_t;
    if (check_mask != bb::full)
        return;
    const cr crs = pos.crs();
    if (crs.mask() == 0) return;
    for (const auto side : {KINGSIDE, QUEENSIDE})
    {
        if (cr::mask(cr::type(c, side)) & crs.mask())
        {
            auto [from, to] = cr::king_move(cr::type(c, side));
            bitboard_t line = bb::from_to_incl(from, to);
            bool safe  = true;
            while (line)
            {
                if (const auto sq = static_cast<square_t>(pop_lsb(line)); pos.attacking_sq_bb(sq) & pos.color_occupancy(~c))
                {
                    safe = false;
                    break;
                }
            }
            if (!safe) continue;
            list.add(move_t::make<CASTLING>(from, to, cr::type(c, side)));
        }
    }


}





#endif
