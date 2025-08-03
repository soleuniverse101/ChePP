//
// Created by paul on 7/29/25.
//

#ifndef MOVE_ORDERING_H
#define MOVE_ORDERING_H

#include "movegen.h"
#include "algorithm"
#include "types.h"



inline int score_move(const move_t& m, const position_t& pos, const move_t prev_best) {
    if (m == prev_best) {
        return 100'000;
    }
    const auto captured = pos.piece_at(m.to_sq());
    const auto attacker = pos.piece_at(m.from_sq());
    const auto from_attackers = pos.attacking_sq_bb(m.from_sq()) & pos.color_occupancy(~pos.color());
    const auto to_attackers = pos.attacking_sq_bb(m.to_sq()) & pos.color_occupancy(~pos.color());

    int score = 0;

    if (captured != NO_PIECE) {
        score += 10'000 + 10 * piece_value(captured) - piece_value(attacker);
    }

    if (m.type_of() == PROMOTION) {
        score += 8'000 + piece_value(static_cast<piece_t>(m.promotion_type()));
    }

    if (m.type_of() == CASTLING) {
        score += 20'000;
    }

    const int attacker_val = piece_value(attacker);

    if (from_attackers) {
        int min_enemy_val = 10000;
        bitboard_t b = from_attackers;
        while (b) {
            square_t sq = static_cast<square_t>(pop_lsb(b));
            piece_t p = pos.piece_at(sq);
            min_enemy_val = std::min(min_enemy_val, piece_value(p));
        }
        int diff = attacker_val - min_enemy_val;
        if (diff < 0) diff = 0;
        score += 5 * diff;
    }

    if (to_attackers) {
        int min_enemy_val = 10000;
        bitboard_t b = to_attackers;
        while (b) {
            square_t sq = static_cast<square_t>(pop_lsb(b));
            piece_t p = pos.piece_at(sq);
            min_enemy_val = std::min(min_enemy_val, piece_value(p));
        }
        int diff = min_enemy_val - attacker_val;
        if (diff < 0) diff = 0;
        score -= 7 * diff;
    }

    if (bitboard_t b = bb::attacks(piece_piece_type(attacker), m.to_sq()) & pos.color_occupancy(~pos.color())) {
        int max_enemy_val = 0;
        while (b) {
            square_t sq = static_cast<square_t>(pop_lsb(b));
            piece_t p = pos.piece_at(sq);
            max_enemy_val = std::max(max_enemy_val, piece_value(p));
        }
        int diff = max_enemy_val - attacker_val;
        if (diff < 0) diff = 0;
        score += 7 * diff;
    }


    return score;
}

inline void order_moves(const position_t& pos, move_list_t& list, const move_t prev_best = move_t::null()) {
    std::vector<std::pair<int, move_t>> scored;
    scored.reserve(list.size());

    for (const auto& m : list) {
        scored.emplace_back(score_move(m, pos, prev_best), m);
    }

    std::ranges::sort(scored, [](auto& a, auto& b) {
        return a.first > b.first;
    });

    for (size_t i = 0; i < list.size(); ++i) {
        list[i] = scored[i].second;
    }
}


#endif //MOVE_ORDERING_H
