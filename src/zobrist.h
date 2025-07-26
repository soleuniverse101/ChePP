#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "castle.h"
#include "move.h"
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

    static void init(std::uint64_t seed);

    [[nodiscard]] hash_t value() const { return m_hash; }

    template <move_type_t move_type>
    void play_move(move_t move, const position_t& pos);


private:
    void flip_piece(const piece_t pt, const square_t sq)
    {
        m_hash ^= s_psq.at(pt).at(sq);
        //m_hash ^= s_psq.at(pt)
    }
    void move_piece(const piece_t pt, const square_t from, const square_t to)
    {
        flip_piece(pt, from);
        flip_piece(pt, to);
    }
    void promote_piece(const color_t c, const piece_type_t pt, const square_t sq)
    {
        flip_piece(piece(PAWN, c), sq);
        flip_piece(piece(pt, c), sq);
    }
    void flip_castling_rights(uint8_t mask)
    {

        while (const int idx = pop_lsb(mask) < NB_CASTLING_TYPES)
        {
            m_hash ^= s_castling.at(idx);
        }
        /**
        for (const auto t : castling_types)
        {
            if (mask & castling_rights_t::mask(t)) m_hash ^= s_castling.at(t);
        }
        **/

    }

    static all_pieces<all_squares<hash_t>> s_psq;
    static all_files<hash_t>                    s_ep;
    static all_castling_types<hash_t>          s_castling;
    static hash_t                               s_side, s_no_pawns;
    hash_t m_hash{};
};

#endif //ZOBRIST_H
