#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include "bitboard.h"
#include "move.h"
#include "prng.h"
#include "types.h"
#include "zobrist.h"
#include "castle.h"

class state_t
{
  public:
    state_t*          m_previous{};
    square_t          m_ep_square;
    all_colors<bitboard_t>        m_checkers{};
    all_colors<bitboard_t>        m_blockers{};
    all_colors<bitboard_t>        m_check_mask{};
    castling_rights_t m_crs;
};

class position_t
{
  public:
    all_squares<piece_t> m_pieces{};
    all_colors<all_piece_types<bitboard_t>> m_pieces_occupancy{};
    all_colors<bitboard_t>                  m_color_occupancy{};
    bitboard_t                              m_global_occupancy{};
    state_t*                                m_state{};
    color_t m_color{};
    zobrist_t hash;


    [[nodiscard]] piece_t piece_at(const square_t sq) const { return m_pieces.at(sq); }
    [[nodiscard]] piece_type_t piece_type_at(const square_t sq) const { return piece_type(m_pieces.at(sq)); }
    [[nodiscard]] bool is_occupied(const square_t sq) const { return m_global_occupancy & bb::sq_mask(sq); }
    [[nodiscard]] color_t color() const { return m_color; }
    template <class... pieces>
    bitboard_t pieces_bb(color_t c, piece_type_t first, const pieces... rest) const;
    template <class... pieces>
    bitboard_t               pieces_bb(piece_type_t first, const pieces... rest) const;
    [[nodiscard]] bitboard_t attacking_sq_bb(square_t sq) const;

    [[nodiscard]] bitboard_t pieces_occupancy(const color_t c, const piece_type_t p) const
    {
        return m_pieces_occupancy.at(c).at(p);
    }
    [[nodiscard]] bitboard_t color_occupancy(const color_t c) const
    {
        if (c == ANY)
        {
            return m_global_occupancy;
        }
        return m_color_occupancy.at(c);
    }
    [[nodiscard]] bitboard_t        checkers(const color_t c) const { return m_state->m_checkers.at(c); }
    [[nodiscard]] bitboard_t        blockers(const color_t c) const { return m_state->m_blockers.at(c); }
    [[nodiscard]] square_t          ep_square() const { return m_state->m_ep_square; }
    [[nodiscard]] castling_rights_t crs() const { return m_state->m_crs; }

    void update_checkers(color_t c) const;
};

template <class... pieces>
bitboard_t position_t::pieces_bb(const color_t c, const piece_type_t first,
                                 const pieces... rest) const
{
    if constexpr (sizeof...(rest) == 0)
    {
        return m_pieces_occupancy[c][first];
    }
    else
    {
        return m_pieces_occupancy[c][first] | get_pieces_bitboard(c, rest...);
    }
}
template <class... pieces>
bitboard_t position_t::pieces_bb(const piece_type_t first, const pieces... rest) const
{
    return pieces_bb(WHITE, first, rest...) | pieces_bb(BLACK, first, rest...);
}
inline bitboard_t position_t::attacking_sq_bb(const square_t sq) const
{
    return (bb::attacks<QUEEN>(sq, m_global_occupancy) & pieces_bb(QUEEN)) |
           (bb::attacks<ROOK>(sq, m_global_occupancy) & pieces_bb(ROOK)) |
           (bb::attacks<BISHOP>(sq, m_global_occupancy) & pieces_bb(BISHOP)) |
           (bb::attacks<KNIGHT>(sq, m_global_occupancy) & pieces_bb(KNIGHT)) |
           (bb::attacks<PAWN>(sq, m_global_occupancy, WHITE) & pieces_bb(WHITE, PAWN)) |
           (bb::attacks<PAWN>(sq, m_global_occupancy, BLACK) & pieces_bb(BLACK, PAWN)) |
           (bb::attacks<KING>(sq, m_global_occupancy) & pieces_bb(KING));
}

inline void position_t::update_checkers(const color_t c) const
{
    const auto ksq = static_cast<square_t>(get_lsb(pieces_bb(c, KING)));
    //
    m_state->m_check_mask.at(c) =m_state->m_checkers.at(c) = attacking_sq_bb(ksq) & color_occupancy(~c);
    m_state->m_blockers.at(c) = bb::empty;
    for (const auto piece : {BISHOP, ROOK, QUEEN})
    {
        auto enemies = pieces_bb(~c, piece);
        while (enemies)
        {
            const auto sq      = static_cast<square_t>(pop_lsb(enemies));
            if (const auto line = bb::from_to_excl(sq, ksq); popcount(line) == 1)
            {
                m_state->m_blockers.at(c) |= line & color_occupancy(ANY);
                m_state->m_check_mask.at(c) |= line;
            }
        }
    }

}



#endif
