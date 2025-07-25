#include "zobrist.h"
#include "position.h"

using zb = zobrist_t;

all_pieces<all_squares<hash_t>> zb::s_psq{};
all_files<hash_t>                    zb::s_ep{};
all_castling_rights<hash_t>          zb::s_castling{};
hash_t                               zb::s_side, zb::s_no_pawns;

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


//to be called before the move is reflected on the board
void zobrist_t::play_move(const move_t move, const position_t& pos)
{
    m_hash ^= s_side; // change color
    m_hash ^= s_ep.at(pos.ep_square()); //reset old ep square

    const square_t from = move.from_sq();
    const square_t     to   = move.to_sq();
    const piece_t pc = pos.piece_at(from);
    const direction_t up = pos.color() == WHITE ? NORTH : SOUTH;

    if (move.type_of() == CASTLING)
    {

        flip_piece(piece(ROOK, pos.color()), from);
        return;
    }

    move_piece(pc, from, to);
    if (move.type_of() == NORMAL)
    {
        if (pos.is_occupied(to)) //capture
        {
            flip_piece(pos.piece_at(to), to);
        }
        if (pc == PAWN && ((to - from) == (2 * up))) // set new ep square
        {
            m_hash ^= s_ep.at(rk_of(to));
        }
    }
    if (move.type_of() == EN_PASSANT)
    {
        flip_piece(pos.piece_at(to), static_cast<square_t>(to + up)); // capture one up
    }
    if (move.type_of() == PROMOTION)
    {
        promote_piece(pos.color(), move.promotion_type(), move.to_sq());
    }

}