#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include "magics.h"

class state_t
{
  public:
    state_t* previous;
    square_t ep_square;
};

class position_t
{
  public:
    all_colors<all_piece_types<bitboard_t>> pieces_occupancy;
    all_colors<bitboard_t>                  color_occupancy;
    bitboard_t                              global_occupancy;
    state_t*                                state;

    template <typename... pieces>
    bitboard_t pieces_bb(const color_t c, const piece_type_t first, const pieces... rest) const;
    template <typename... pieces>
    bitboard_t pieces_bb(const piece_type_t first, const pieces... rest) const;
    bitboard_t attacking_sq_bb(const square_t sq) const;
};

template <typename... pieces>
bitboard_t position_t::pieces_bb(const color_t color, const piece_type_t first,
                                 const pieces... rest) const
{
    if constexpr (sizeof...(rest) == 0)
    {
        return pieces_occupancy[color][first];
    }
    else
    {
        return pieces_occupancy[color][first] | get_pieces_bitboard(color, rest...);
    }
}
template <typename... pieces>
bitboard_t position_t::pieces_bb(const piece_type_t first, const pieces... rest) const
{
    return pieces_bb(WHITE, first, rest...) | pieces_bb(BLACK, first, rest...);
}
inline bitboard_t position_t::attacking_sq_bb(const square_t sq) const
{
    return bb::attacks<QUEEN>(sq, global_occupancy) & pieces_bb(QUEEN) |
           bb::attacks<ROOK>(sq, global_occupancy) & pieces_bb(ROOK) |
           bb::attacks<BISHOP>(sq, global_occupancy) & pieces_bb(BISHOP) |
           bb::attacks<KNIGHT>(sq, global_occupancy) & pieces_bb(KNIGHT) |
           bb::attacks<PAWN>(sq, global_occupancy, WHITE) & pieces_bb(WHITE, PAWN) |
           bb::attacks<PAWN>(sq, global_occupancy, BLACK) & pieces_bb(BLACK, PAWN) |
           bb::attacks<KING>(sq, global_occupancy) & pieces_bb(KING);
}
#endif
