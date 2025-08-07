#include "ChePP/engine/zobrist.h"
#include "ChePP/engine/position.h"
#include "iostream"

using zb = zobrist_t;

enum_array<piece_t, enum_array<square_t, hash_t>> zb::s_psq{};
enum_array<file_t, hash_t> zb::s_ep{};
all_castling_types<hash_t>      zb::s_castling{};
hash_t                          zb::s_side, zb::s_no_pawns;

void zobrist_t::init(const std::uint64_t seed)
{
    PRNG prng(seed);
    for (auto& pt : s_psq)
    {
        for (auto& sq : pt)
        {
            sq = prng.rand<hash_t>();
        }
    }
    for (auto& ep : s_ep)
    {
        ep = prng.rand<hash_t>();
    }
    for (auto& cs : s_castling)
    {
        cs = prng.rand<hash_t>();
    }
    s_side     = prng.rand<hash_t>();
    s_no_pawns = prng.rand<hash_t>();
}

// to be called before the move is reflected on the board
void zobrist_t::play_move(const move_t move, const position_t& pos)
{
    m_hash ^= s_side;                   // change color
    if (pos.ep_square() != NO_SQUARE)
    {
        // ep square should be set only if it is playable
        // aka if a piece can play en passant
        m_hash ^= s_ep.at(pos.ep_square().file()); // reset old ep square
    }

    const square_t    from  = move.from_sq();
    const square_t    to    = move.to_sq();
    const piece_t     pc    = pos.piece_at(from);
    const piece_type_t pt = pos.piece_type_at(from);
    const color_t     color = pos.color();
    const direction_t up    = pos.color() == WHITE ? NORTH : SOUTH;
    const move_type_t move_type = move.type_of();
    // are we losing castling rights?
    if (const uint8_t lost = (castling_rights_t::lost_by_moving_from(from) | castling_rights_t::lost_by_moving_from(to)) & pos.crs().mask()) [[unlikely]]
    {
        flip_castling_rights(lost);
    }
    //are we castling?
    if  (move_type == CASTLING)
    {
        const auto castling_type = move.castling_type();
        auto [k_from, k_to]                 = castling_rights_t::king_move(castling_type);
        auto [r_from, r_to]                 = castling_rights_t::rook_move(castling_type);

        move_piece(piece_t{color, KING}, k_from, k_to);
        move_piece(piece_t{color, ROOK}, r_from, r_to);
        return; //no capture, no promotion
    }

    move_piece(pc, from, to);
    if  (move_type == NORMAL)
    {
        // capture
        if (pos.is_occupied(to))
        {
            flip_piece(pos.piece_at(to), to);
        }
        // set new ep square
        else if (pt == PAWN && (value_of(to - from) == value_of(2 * up)))
        {
            m_hash ^= s_ep.at(to.file());
        }
    }
    if  (move_type == EN_PASSANT)
    {
        // remove ep pawn
        flip_piece(piece_t{~color, PAWN}, to - up);
    }
    if  (move_type == PROMOTION)
    {
        //remove pawn add promoted type
        promote_piece(pos.color(), move.promotion_type(), move.to_sq());
    }
}

zb::zobrist_t(const position_t& pos)
{
    m_hash = 0;
    for (auto sq = A1; sq <= H8; sq = ++sq)
    {
        if (pos.piece_at(sq) != NO_PIECE)
        {
            flip_piece(pos.piece_at(sq), sq);
        }
    }
    if (pos.ep_square() != NO_SQUARE)
    {
        flip_ep(pos.ep_square().file());
    }
    if (pos.color() == BLACK)
    {
        flip_color();
    }
    flip_castling_rights(~pos.crs().mask());
}
