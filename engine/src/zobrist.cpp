#include "ChePP/engine/zobrist.h"
#include "ChePP/engine/position.h"
#include "iostream"

using zb = zobrist_t;

enum_array<piece_t, enum_array<square_t, hash_t>> zb::s_psq{};
enum_array<file_t, hash_t> zb::s_ep{};
enum_array<castling_type_t, hash_t>      zb::s_castling{};
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
    flip_castling_rights(pos.crs().mask());
}
