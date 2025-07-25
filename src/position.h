#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED
#include "bitboard.h"
#include "types.h"




// the keys used for the Zobrist hash
// need some for all (piece type, square) couple
// for en passant (one of reach file)
// fo rthe castling rights
// for the side whiose turn it is to play
// for testing if there are pawns left
namespace Zobrist {
    hash_t psq[NB_PIECE_TYPE][NB_SQUARES];
    hash_t enpassant[NB_FILES];
    hash_t castling[CASTLING_RIGHT_NB];
    hash_t side, noPawns;
}




class state_t
{
  public:
    state_t* m_previous;
    square_t m_ep_square;
    bitboard_t m_checkers;
    bitboard_t m_blockers;
    castling_rights_t m_crs;
};

class position_t
{
  public:
    all_colors<all_piece_types<bitboard_t>> m_pieces_occupancy;
    all_colors<bitboard_t>                  m_color_occupancy;
    bitboard_t                              m_global_occupancy;
    state_t*                                m_state;


    template <class... pieces>
    bitboard_t pieces_bb(color_t c, piece_type_t first, const pieces... rest) const;
    template <typename... pieces>
    bitboard_t pieces_bb(const piece_type_t first, const pieces... rest) const;
    bitboard_t attacking_sq_bb(const square_t sq) const;

    bitboard_t pieces_occupancy(const color_t c, const piece_type_t p) const { return m_pieces_occupancy.at(c).at(p);}
    bitboard_t color_occupancy(const color_t c) const
    {
        if constexpr (c == ANY)
        {
            return m_global_occupancy;
        }
        return m_color_occupancy.at(c);
    }
    bitboard_t checkers() const { return m_state->m_checkers;}
    bitboard_t blockers() const { return m_state->m_blockers;}
    square_t ep_square() const { return m_state->m_ep_square;}
    castling_rights_t crs() const { return m_state->m_crs;}
};

template <class ... pieces>
bitboard_t position_t::pieces_bb(const color_t c, const piece_type_t first,
                                 const pieces... rest) const
{
    if constexpr (sizeof...(rest) == 0)
    {
        return pieces_occupancy[c][first];
    }
    else
    {
        return pieces_occupancy[c][first] | get_pieces_bitboard(c, rest...);
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
