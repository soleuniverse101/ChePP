//
// Created by paul on 7/29/25.
//

#ifndef MOVE_ORDERING_H
#define MOVE_ORDERING_H

#include "movegen.h"
#include "algorithm"
#include "types.h"



inline int score_move(const move_t& m, const position_t& pos) {
    const auto captured = pos.piece_at(m.to_sq());
    const auto attacker = pos.piece_at(m.from_sq());

    int score = 0;

    if (captured != NO_PIECE) {
        score += 10'000 + 10 * piece_value(captured) - piece_value(attacker);
    }

    if (m.type_of() == PROMOTION) {
        score += 8'000 + piece_value(static_cast<piece_t>(m.promotion_type()));
    }

    if (m.type_of() == CASTLING)
    {
        score += 100'000;
    }

    return score;
}

inline void order_moves(const position_t& pos, move_list_t& list) {
    std::vector<std::pair<int, move_t>> scored;
    scored.reserve(list.size());

    for (const auto& m : list) {
        scored.emplace_back(score_move(m, pos), m);
    }

    std::ranges::sort(scored, [](auto& a, auto& b) {
        return a.first > b.first;
    });

    for (size_t i = 0; i < list.size(); ++i) {
        list[i] = scored[i].second;
    }
}


#endif //MOVE_ORDERING_H
