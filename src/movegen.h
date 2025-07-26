#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include "bitboard.h"
#include "move.h"
#include "position.h"

#include <iostream>
#include <memory>

struct move_list_t
{
    move_list_t() : end(0) {}
    static constexpr size_t max_moves = 256;
    void                    add(const move_t m)
    {
        assert(end < max_moves && "move_list_t overflow");
        moves[end++] = m;
    }

    const move_t& operator[](const size_t index) const
    {
        assert(index < end);
        return moves[index];
    }

    move_t& operator[](const size_t index) {
        assert(index < end);
        return moves[index];
    }

    void clear() { end = 0; }

    void shrink(const size_t n) {end -= n;}

    [[nodiscard]] size_t size() const { return end; }

    std::array<move_t, max_moves> moves;
    size_t                       end;
};

template <color_t c>
void gen_pawn_moves(const position_t& pos, move_list_t& list)
{
    // check mask refers to king attackers u rays of king attackers
    constexpr direction_t up       = c == WHITE ? NORTH : SOUTH;
    constexpr direction_t right    = c == WHITE ? EAST : WEST;
    constexpr auto        down     = static_cast<direction_t>(-up);
    constexpr auto        up_right = static_cast<direction_t>(up + right);
    constexpr auto        up_left  = static_cast<direction_t>(up - right);

    constexpr bitboard_t bb_promotion_rank = c == WHITE ? bb::rk_mask(RANK_7) : bb::rk_mask(RANK_2);
    constexpr bitboard_t bb_third_rank     = c == WHITE ? bb::rk_mask(RANK_3) : bb::rk_mask(RANK_6);
    const bitboard_t     check_mask        = pos.check_mask(c) == bb::empty ? bb::full : pos.check_mask(c);
    const bitboard_t     available         = ~pos.color_occupancy(ANY);
    const bitboard_t     enemy             = pos.color_occupancy(~c);
    const bitboard_t     pawns             = pos.pieces_bb(c, PAWN);
    const bitboard_t     ep_bb             = (pos.ep_square() == NO_SQUARE ? bb::empty : bb::sq_mask(pos.ep_square()));
    // straight
    {
        bitboard_t single_push = bb::shift<up>(pawns & ~bb_promotion_rank) & available;
        bitboard_t double_push = bb::shift<up>(single_push & bb_third_rank) & available & check_mask;
        single_push &= check_mask;
        while (single_push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(single_push));
            list.add(move_t::make<NORMAL>(static_cast<square_t>(sq - static_cast<int>(up)), sq));
        }
        while (double_push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(double_push));
            list.add(move_t::make<NORMAL>(static_cast<square_t>(sq - 2 * static_cast<int>(up)), sq));
        }
    }
    // promotion
    if (const bitboard_t promotions = pawns & bb_promotion_rank)
    {
        bitboard_t push = bb::shift<up>(promotions) & available & check_mask;
        while (push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(push));
            list.add(move_t::make<PROMOTION>(static_cast<square_t>(sq - static_cast<int>(up)), sq));
        }
        bitboard_t take_right = bb::shift<up_right>(promotions) & enemy & check_mask;
        while (take_right)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_right));
            list.add(move_t::make<PROMOTION>(static_cast<square_t>(sq - (up + right)), sq));
        }
        bitboard_t take_left = bb::shift<up_left>(promotions) & enemy & check_mask;
        while (take_left)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_left));
            list.add(move_t::make<PROMOTION>(static_cast<square_t>(sq - (up - right)), sq));
        }
    }
    // capture
    {

        bitboard_t capturable = enemy | ep_bb;
        bitboard_t ep_mask    = bb::empty;
        if (check_mask & bb::shift<down>(ep_bb))
        {
            ep_mask = ep_bb;
        }
        bitboard_t take_right = bb::shift<up_right>(pawns & ~bb_promotion_rank) & capturable & (check_mask | ep_mask);
        while (take_right)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(take_right)); sq == pos.ep_square()) [[unlikely]]
            {
                list.add(move_t::make<EN_PASSANT>(static_cast<square_t>(sq - (up + right)), sq));
            }
            else [[likely]]
            {
                list.add(move_t::make<NORMAL>(static_cast<square_t>(sq - (up + right)), sq));
            }
        }
        bitboard_t take_left = bb::shift<up_left>(pawns & ~bb_promotion_rank) & capturable & (check_mask | ep_mask);
        while (take_left)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(take_left)); sq == pos.ep_square()) [[unlikely]]
            {
                list.add(move_t::make<EN_PASSANT>(static_cast<square_t>(sq - (up - right)), sq));
            }
            else [[likely]]
            {
                list.add(move_t::make<NORMAL>(static_cast<square_t>(sq - (up - right)), sq));
            }
        }
    }
}



template <piece_type_t pc, color_t c>
void gen_pc_moves(const position_t& pos, move_list_t& list)
{
    const bitboard_t check_mask = pos.check_mask(c) == bb::empty ? bb::full : pos.check_mask(c);
    bitboard_t       bb         = pos.pieces_occupancy(c, pc);
    while (bb)
    {
        auto       from    = static_cast<square_t>(pop_lsb(bb));
        bitboard_t attacks = bb::attacks<pc>(from, pos.color_occupancy(ANY)) & ~pos.color_occupancy(c) & check_mask;
        while (attacks)
        {
            list.add(move_t::make<NORMAL>(from, static_cast<square_t>(pop_lsb(attacks))));
        }
    }
}

template <color_t c>
void gen_castling(const position_t& pos, move_list_t& list)
{
    const bitboard_t check_mask = pos.check_mask(c) == bb::empty ? bb::full : pos.check_mask(c);
    using cr                    = castling_rights_t;
    if (check_mask != bb::full)
        return;
    const cr crs = pos.crs();
    if (crs.mask() == 0)
        return;
    for (const auto side : {KINGSIDE, QUEENSIDE})
    {
        if (cr::mask(cr::type(c, side)) & crs.mask())
        {
            auto [from, to]       = cr::king_move(cr::type(c, side));
            const direction_t dir = direction_from(from, to);
            assert(dir != INVALID);
            bool safe = true;
            for (square_t sq = from; sq != to; sq = static_cast<square_t>(sq + static_cast<int>(dir)))
            {
                if (pos.piece_at(sq) != NO_PIECE || pos.attacking_sq_bb(sq) & pos.color_occupancy(~c))
                {
                    safe = false;
                    break;
                }
            }
            if (safe)
            {
                list.add(move_t::make<CASTLING>(from, to, cr::type(c, side)));
            }
        }
    }
}


template <color_t c>
void gen_king_moves(const position_t& pos, move_list_t& list)
{
    const auto       from  = static_cast<square_t>(get_lsb(pos.pieces_bb(c, KING)));
    const bitboard_t       moves      = bb::attacks<KING>(from, pos.color_occupancy(ANY));
    bitboard_t quiet = moves & ~pos.color_occupancy(ANY);
    while (quiet)
    {
        const auto to = static_cast<square_t>(pop_lsb(quiet));
        list.add(move_t::make<NORMAL>(from, to, KING));
    }
    bitboard_t captures = moves & pos.color_occupancy(~c);
    while (captures)
    {
        const auto to = static_cast<square_t>(pop_lsb(captures));
        list.add(move_t::make<NORMAL>(from, to, KING));
    }
    gen_castling<c>(pos, list);
}

template <color_t c>
void gen_legal(const position_t& pos, move_list_t& list)
{

    gen_pawn_moves<c>(pos, list);
    gen_pc_moves<BISHOP, c>(pos, list);
    gen_pc_moves<KNIGHT, c>(pos, list);
    gen_pc_moves<ROOK, c>(pos, list);
    gen_pc_moves<QUEEN, c>(pos, list);
    gen_king_moves<c>(pos, list);
    size_t idx = 0;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (pos.is_legal<c>(list[i]))
        {
            list[idx++] = list[i];
        }
    }
    list.shrink(list.size() - idx);
}

#endif
