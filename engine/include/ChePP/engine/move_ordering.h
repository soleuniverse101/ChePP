#ifndef MOVE_ORDERING_H
#define MOVE_ORDERING_H

#include "algorithm"
#include "movegen.h"
#include "types.h"
#include <array>
#include <ranges>
#include <vector>



inline int mvv_lva_score(const Piece attacker, const Piece victim) {
    static constexpr EnumArray<PieceType, EnumArray<PieceType, int>> table = {
        EnumArray<PieceType, int>{105, 205, 305, 405, 505, 605},
        {104, 204, 304, 404, 504, 604},
        {103, 203, 303, 403, 503, 603},
        {102, 202, 302, 402, 502, 602},
        {101, 201, 301, 401, 501, 601},
        {100, 200, 300, 400, 500, 600}
    };
    return table[attacker.type()][victim.type()];
}

inline bool see_ge(const Position& pos, const Move m, const int threshold) {
    const Square to   = m.to_sq();
    const Square from = m.from_sq();

    const Piece attacker = pos.piece_at(from);
    const Piece victim   = pos.piece_at(to);

    int gain[32];
    int depth = 0;

    gain[0] = victim.piece_value();
    if (gain[0] < threshold) return false;


    Bitboard occ = pos.occupancy();
    Bitboard attackers = pos.attacking_sq_bb(to);

    occ &= ~Bitboard(from);
    attackers &= occ;

    Color stm = ~attacker.color();

    int target_value = attacker.piece_value();

    while (true) {
        const Bitboard att = attackers & pos.color_occupancy(stm);
        if (!att) break;

        Square sq_att{NO_SQUARE};
        Piece  p_att{NO_PIECE};
        int      p_att_val = KING.piece_value();

        Bitboard bb = att;
        while (bb) {
            const Square s{bb.pop_lsb()};
            const Piece  p = pos.piece_at(s);
            const int      v = p.piece_value();
            if (v < p_att_val) {
                p_att_val = v;
                p_att     = p;
                sq_att    = s;
            }
        }


        ++depth;
        if (depth >= 31) break;
        gain[depth] = -gain[depth - 1] + target_value;

        if (gain[depth] < threshold) break;


        target_value = p_att_val;

        occ &= ~Bitboard(sq_att);

        attackers = pos.attacking_sq_bb(to, occ);
        attackers &= occ;

        stm = ~stm;
    }

    for (int i = depth - 1; i >= 0; --i) {
        gain[i] = -std::max(-gain[i], gain[i + 1]);
    }


    return gain[0] >= threshold;
}


inline int quiet_score(const Move& m,
                       const Move prev_best,
                       const std::array<Move, 2>& killers,
                       EnumArray<Square, EnumArray<Square, int>>& history) {
    if (m == prev_best) return 100'000;
    if (m == killers[0] || m == killers[1]) return 50'000;
    if (m.type_of() == PROMOTION) return 45000;
    return history.at(m.from_sq()).at(m.to_sq());
}

inline int score_move(const Move& m, const Position& pos,
                      const Move prev_best,
                      const std::array<Move, 2>& killers,
                      EnumArray<Square, EnumArray<Square, int>>& history) {
    const auto captured = pos.piece_at(m.to_sq());
    if (captured != NO_PIECE) {
        const auto attacker = pos.piece_at(m.from_sq());
        return 10'000 + mvv_lva_score(attacker, captured);
    }
    return quiet_score(m, prev_best, killers, history);
}

inline void order_moves(const Position& pos,
                        MoveList& list,
                        const Move prev_best,
                        EnumArray<Square, EnumArray<Square, int>>& history,
                        const std::array<Move, 2>& killers) {
    std::vector<Move> good_captures, bad_captures, quiets;
    good_captures.reserve(list.size());
    bad_captures.reserve(list.size());
    quiets.reserve(list.size());

    int idx = 0;
    for (const auto& m : list) {
        const auto captured = pos.piece_at(m.to_sq());

        if (m == prev_best) {
            list[idx++] = m;
        }
        else if (captured != NO_PIECE) {
            if (see_ge(pos, m, -30))
                good_captures.push_back(m);
            else
                bad_captures.push_back(m);
        }
        else {
            quiets.push_back(m);
        }
    }

    // Sort good captures by MVV-LVA
    std::ranges::sort(good_captures, [&](const Move& a, const Move& b) {
        const auto attacker_a = pos.piece_at(a.from_sq());
        const auto victim_a   = pos.piece_at(a.to_sq());
        const auto attacker_b = pos.piece_at(b.from_sq());
        const auto victim_b   = pos.piece_at(b.to_sq());
        return mvv_lva_score(attacker_a, victim_a) >
               mvv_lva_score(attacker_b, victim_b);
    });

    std::ranges::sort(quiets, [&](const Move& a, const Move& b) {
        return quiet_score(a, prev_best, killers, history) >
               quiet_score(b, prev_best, killers, history);
    });

    std::ranges::sort(bad_captures, [&](const Move& a, const Move& b) {
        const auto attacker_a = pos.piece_at(a.from_sq());
        const auto victim_a   = pos.piece_at(a.to_sq());
        const auto attacker_b = pos.piece_at(b.from_sq());
        const auto victim_b   = pos.piece_at(b.to_sq());
        return mvv_lva_score(attacker_a, victim_a) >
               mvv_lva_score(attacker_b, victim_b);
    });

    list.shrink(list.size() - idx);
    for (const auto& m : good_captures) list.push_back(m);
    for (const auto& m : quiets)        list.push_back(m);
    for (const auto& m : bad_captures)  list.push_back(m);
}

#endif // MOVE_ORDERING_H
