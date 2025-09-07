#ifndef MOVE_ORDERING_H
#define MOVE_ORDERING_H

#include "movegen.h"
#include "types.h"

#include <algorithm>
#include <ranges>

inline int mvv_lva_score(const Piece attacker, const Piece victim) {
    return (victim.piece_value() - attacker.piece_value());
}

inline void score_moves(const Position& pos,
                        MoveList& list,
                        const Move prev_best, const SearchStackNode& ssNode)
{
    for (auto& [move, score] : list) {

        if (move == prev_best) {
            score = 10000;
        } else if (move == ssNode.killer1) {
            score = 9000;
        } else if (move == ssNode.killer2) {
            score = 8900;
        } else if (move.type_of() == PROMOTION) {
            score = move.promotion_type().piece_value() * 8;
        } else {
            auto victim   = pos.piece_at(move.to_sq());
            const auto attacker = pos.piece_at(move.from_sq());

            if (victim != NO_PIECE || move.type_of() == EN_PASSANT) {
                score = pos.see(move) * 10;
            } else {
                score = ssNode.history[move.from_sq()][move.to_sq()];
                auto lva = KING;
            }
        }
    }
}


#endif // MOVE_ORDERING_H
