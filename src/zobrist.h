#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "move.h"
#include "prng.h"
#include "types.h"

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
    zobrist_t() : m_hash(0) {}
    explicit zobrist_t(const hash_t hash) : m_hash(hash) {}
    static void init(std::uint64_t seed);

    void play_move(move_t move, const position_t& pos);

private:
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
        flip_piece(piece(PAWN, c), sq);
        flip_piece(piece(pt, c), sq);
    }

    static all_pieces<all_squares<hash_t>> s_psq;
    static all_files<hash_t>                    s_ep;
    static all_castling_rights<hash_t>          s_castling;
    static hash_t                               s_side, s_no_pawns;
    hash_t m_hash;
};

#endif //ZOBRIST_H
