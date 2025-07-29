//
// Created by paul on 7/29/25.
//

#ifndef EVAL_H
#define EVAL_H

#include "movegen.h"

constexpr int pawn_table[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, -20, -20, 10, 10, 5,
    5, -5, -10, 0, 0, -10, -5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, 5, 10, 25, 25, 10, 5, 5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
    0, 0, 0, 0, 0, 0, 0, 0
};

constexpr int knight_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,0,0,0,0,-20,-40,
    -30,0,10,15,15,10,0,-30,
    -30,5,15,20,20,15,5,-30,
    -30,0,15,20,20,15,0,-30,
    -30,5,10,15,15,10,5,-30,
    -40,-20,0,5,5,0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

template <color_t us>
int evaluate_position(const position_t& pos) {
    int score = 0;

    for (const color_t c : {WHITE, BLACK}) {
        int multiplier = (c == us) ? 1 : -1;

        for (const piece_type_t pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN}) {
            int pieceCount = popcount(pos.pieces_bb(c, pt));
            score += multiplier * pieceCount * piece_value(pt);
        }

        auto pawns_bb = pos.pieces_bb(c, PAWN);
        while (pawns_bb) {
            int sq = pop_lsb(pawns_bb);

            int table_sq = (c == WHITE) ? sq : mirror_sq(sq);
            score += multiplier * pawn_table[table_sq];
        }

        auto knights_bb = pos.pieces_bb(c, KNIGHT);
        while (knights_bb) {
            int sq = pop_lsb(knights_bb);
            int table_sq = (c == WHITE) ? sq : mirror_sq(sq);
            score += multiplier * knight_table[table_sq];
        }
    }

    return score;
}


#endif //EVAL_H
