#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "prng.h"
#include "types.h"

#include <bits/ios_base.h>

class position_t;

// the keys used for the Zobrist hash
// need some for all (piece type, square) couple
// for en passant (one of reach file)
// fo the castling rights
// for the side whose turn it is to play
// for testing if there are pawns left
class zobrist_t
{
public:
    explicit zobrist_t(const position_t& pos);
    explicit zobrist_t(const hash_t hash) : m_hash(hash) {}
    zobrist_t(const zobrist_t& other) = default;

    static void init(std::uint64_t seed);

    [[nodiscard]] hash_t value() const { return m_hash; }


    void flip_piece(const piece_t pt, const square_t sq)
    {
        m_hash ^= s_psq.at(pt).at(sq);
    }
    void move_piece(const piece_t pt, const square_t from, const square_t to)
    {
        flip_piece(pt, from);
        flip_piece(pt, to);
    }
    void promote_piece(const color_t c, const piece_type_t pt, const square_t sq)
    {
        flip_piece(piece_t{c, PAWN}, sq);
        flip_piece(piece_t{c, pt}, sq);
    }
    void flip_castling_rights(const uint8_t mask)
    {
        /**
        while (const int idx = pop_lsb(mask) < NB_CASTLING_TYPES)
        {
            m_hash ^= s_castling.at(idx);
        }
        **/
        for (auto type = WHITE_KINGSIDE; type <= BLACK_QUEENSIDE; ++type)
        {
            if (mask & type.mask()) m_hash ^= s_castling.at(type);
        }

    }

    void flip_ep(const file_t fl)
    {
        m_hash ^= s_ep.at(fl);
    }

    void flip_color()
    {
        m_hash ^= s_side;
    }

    static enum_array<piece_t, enum_array<square_t, hash_t>> s_psq;
    static enum_array<file_t, hash_t> s_ep;
    static enum_array<castling_type_t, hash_t>          s_castling;
    static hash_t                               s_side, s_no_pawns;
    hash_t m_hash{};
};

#endif //ZOBRIST_H
