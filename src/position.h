#include "bitboard.h"

class state_t
{
};

class position_t
{
  public:
    bitboard_t pieces_occupancy[NB_COLORS][NB_PIECES];
    bitboard_t color_occupancy[NB_COLORS];
    bitboard_t global_occupancy;

    template <typename... pieces>
    bitboard_t pieces_bb(color_t c, piece_t first, pieces... rest);
    template <typename... pieces>
    bitboard_t pieces_bb(piece_t first, pieces... rest);
    bitboard_t attacking_sq_bb(square_t sq);
};

template <typename... pieces>
bitboard_t position_t::pieces_bb(color_t color, piece_t first, pieces... rest)
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
bitboard_t position_t::pieces_bb(piece_t first, pieces... rest)
{
    return pieces_bb(WHITE, first, rest...) | pieces_bb(BLACK, first, rest...);
}
inline bitboard_t position_t::attacking_sq_bb(square_t sq)
{
    return attacks_bb<QUEEN>(sq, global_occupancy) & pieces_bb(QUEEN) |
           attacks_bb<ROOK>(sq, global_occupancy) & pieces_bb(ROOK) |
           attacks_bb<BISHOP>(sq, global_occupancy) & pieces_bb(BISHOP) |
           attacks_bb<KNIGHT>(sq, global_occupancy) & pieces_bb(KNIGHT) |
           attacks_bb<PAWN>(sq, global_occupancy, WHITE) & pieces_bb(WHITE, PAWN) |
           attacks_bb<PAWN>(sq, global_occupancy, BLACK) & pieces_bb(BLACK, PAWN) |
           attacks_bb<KING>(sq, global_occupancy) & pieces_bb(KING);
}
