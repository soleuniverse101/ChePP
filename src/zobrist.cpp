#include "zobrist.h"
#include "position.h"

using zb = zobrist_t;

all_pieces<all_squares<hash_t>> zb::s_psq{};
all_files<hash_t>               zb::s_ep{};
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
    m_hash ^= s_ep.at(pos.ep_square()); // reset old ep square

    const square_t    from  = move.from_sq();
    const square_t    to    = move.to_sq();
    const piece_t     pc    = pos.piece_at(from);
    const color_t     color = pos.color();
    const direction_t up    = pos.color() == WHITE ? NORTH : SOUTH;

    if (const int8_t lost = castling_rights_t::lost_by_moving_from(from)) [[unlikely]]
    {
        castling_rights(pos.crs().mask() & lost);
    }
    if (move.type_of() == CASTLING) [[unlikely]]
    {
        const castling_type_t castling_type = move.castling_type();
        auto [k_from, k_to]                 = castling_rights_t::king_move(castling_type);
        auto [r_from, r_to]                 = castling_rights_t::rook_move(castling_type);

        move_piece(piece(KING, color), k_from, k_to);
        move_piece(piece(ROOK, color), r_from, r_to);
        return;
    }

    move_piece(pc, from, to);
    if (move.type_of() == NORMAL) [[likely]]
    {
        // capture
        if (pos.is_occupied(to))
        {
            flip_piece(pos.piece_at(to), to);
        }
        // set new ep square
        if (pc == PAWN && ((to - from) == (2 * up)))
        {
            m_hash ^= s_ep.at(rk_of(to));
        }
    }
    if (move.type_of() == EN_PASSANT) [[unlikely]]
    {
        flip_piece(pos.piece_at(to), static_cast<square_t>(to + up)); // capture one up
    }
    if (move.type_of() == PROMOTION) [[unlikely]]
    {
        promote_piece(pos.color(), move.promotion_type(), move.to_sq());
    }
}