#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include "bitboard.h"
#include "move.h"
#include "position.h"

#include <iostream>
#include <memory>

enum movegen_type_t
{
    ALL,
    TACTICAL
};

struct move_list_t
{
    static constexpr size_t max_moves = 256;

    move_list_t() : m_size(0) {}

    void add(const move_t& m)
    {
        assert(m_size < max_moves && "move_list_t overflow");
        m_moves[m_size++] = m;
    }
    void push_back(const move_t& m) { add(m); }

    const move_t& operator[](const size_t index) const
    {
        assert(index < m_size);
        return m_moves[index];
    }
    move_t& operator[](const size_t index)
    {
        assert(index < m_size);
        return m_moves[index];
    }

    void clear() { m_size = 0; }

    void shrink(const size_t n)
    {
        assert(n <= m_size);
        m_size -= n;
    }

    [[nodiscard]] size_t size() const { return m_size; }

    [[nodiscard]] static constexpr size_t capacity() { return max_moves; }

    [[nodiscard]] bool empty() const { return m_size == 0; }

    move_t* begin() { return m_moves.data(); }
    move_t* end() { return m_moves.data() + m_size; }
    [[nodiscard]] const move_t* begin() const { return m_moves.data(); }
    [[nodiscard]] const move_t* end() const { return m_moves.data() + m_size; }
    [[nodiscard]] const move_t* cbegin() const { return m_moves.data(); }
    [[nodiscard]] const move_t* cend() const { return m_moves.data() + m_size; }

private:
    std::array<move_t, max_moves> m_moves;
    size_t m_size;
};



inline void make_all_promotions(move_list_t& list, const square_t from, const square_t to)
{
    list.add(move_t::make<PROMOTION>(from, to, QUEEN));
    list.add(move_t::make<PROMOTION>(from, to, ROOK));
    list.add(move_t::make<PROMOTION>(from, to, BISHOP));
    list.add(move_t::make<PROMOTION>(from, to, KNIGHT));
}

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
            make_all_promotions(list, static_cast<square_t>(sq - static_cast<int>(up)), sq);

        }
        bitboard_t take_right = bb::shift<up_right>(promotions) & enemy & check_mask;
        while (take_right)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_right));
            make_all_promotions(list, static_cast<square_t>(sq - (up + right)), sq);

        }
        bitboard_t take_left = bb::shift<up_left>(promotions) & enemy & check_mask;
        while (take_left)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_left));
            make_all_promotions(list, static_cast<square_t>(sq - (up - right)), sq);
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
    using cr                    = castling_rights_t;
    if (pos.check_mask(c) != bb::empty)
        return;
    const cr crs = pos.crs();
    if (crs.mask() == 0)
        return;
    for (const auto side : {KINGSIDE, QUEENSIDE})
    {
        if (cr::mask(cr::type(c, side)) & crs.mask())
        {
            auto [k_from, k_to]       = cr::king_move(cr::type(c, side));
            auto [r_from, r_to] = cr::rook_move(cr::type(c, side));
            assert(pos.piece_at(k_from) == piece(KING, c));
            bool safe = ((bb::from_to_excl(k_from, r_from) & pos.color_occupancy(ANY)) == bb::empty);
            const direction_t dir = direction_from(k_from, k_to);
            assert(dir != INVALID);
            const int offset = dir;
            for (auto sq = k_from; sq != k_to && safe; sq = static_cast<square_t>(sq + offset))
            {
                if (pos.attacking_sq_bb(sq) & pos.color_occupancy(~c))
                {
                    safe = false;
                    break;
                }
            }
            if (safe)
            {
                list.add(move_t::make<CASTLING>(k_from, k_to, cr::type(c, side)));
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
        list.add(move_t::make<NORMAL>(from, to));
    }
    bitboard_t captures = moves & pos.color_occupancy(~c);
    while (captures)
    {
        const auto to = static_cast<square_t>(pop_lsb(captures));
        list.add(move_t::make<NORMAL>(from, to));
    }
    gen_castling<c>(pos, list);
}

template <color_t c>
void gen_legal(const position_t& pos, move_list_t& list)
{
    const int n_checkers = popcount(pos.checkers(c));
    assert(n_checkers <= 2);
    if (n_checkers == 2)
    {
        gen_king_moves<c>(pos, list);
    }
    else
    {
        gen_pawn_moves<c>(pos, list);
        gen_pc_moves<BISHOP, c>(pos, list);
        gen_pc_moves<KNIGHT, c>(pos, list);
        gen_pc_moves<ROOK, c>(pos, list);
        gen_pc_moves<QUEEN, c>(pos, list);
        gen_king_moves<c>(pos, list);
    }
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

template <color_t c>
void gen_tactical(const position_t& pos, move_list_t& list)
{
    const int n_checkers = popcount(pos.checkers(c));
    assert(n_checkers <= 2);
    if (n_checkers == 2)
    {
        gen_king_moves<c>(pos, list);
    }
    else
    {
        gen_pawn_moves<c>(pos, list);
        gen_pc_moves<BISHOP, c>(pos, list);
        gen_pc_moves<KNIGHT, c>(pos, list);
        gen_pc_moves<ROOK, c>(pos, list);
        gen_pc_moves<QUEEN, c>(pos, list);
        gen_king_moves<c>(pos, list);
    }
    size_t idx = 0;
    for (size_t i = 0; i < list.size(); ++i)
    {
        move_t mv = list[i];
        if (pos.is_legal<c>(list[i]) && ( pos.is_occupied(mv.to_sq()) || mv.type_of() == PROMOTION))
        {
            list[idx++] = list[i];
        }
    }
    list.shrink(list.size() - idx);
}



inline void perft(position_t& pos, const int ply, size_t& out) {
    move_list_t l;
    if (pos.color() == WHITE)
        gen_legal<WHITE>(pos, l);
    else
        gen_legal<BLACK>(pos, l);

    if (ply == 1)
    {
        out += l.size();
        return;
    }

    for (size_t i = 0; i < l.size(); ++i) {
        const auto mv = l[i];
        pos.do_move(mv);
        //std::cout << mv << std::endl;
        perft(pos, ply - 1, out);
        pos.undo_move(mv);
    }
}

#endif
