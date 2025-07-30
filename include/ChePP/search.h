//
// Created by paul on 7/29/25.
//

#ifndef SEARCH_H
#define SEARCH_H

#include "eval.h"
#include "move_ordering.h"
#include "tt.h"



template <color_t c>
int minimax(position_t& pos, int depth, int alpha, int beta, int& searched, int& tt_hits) {
    constexpr auto opponent = ~c;

    if (auto entry = g_tt.probe(pos.hash(), depth))
    {
        tt_hits++;
        return entry.value().m_score;
    }

    move_list_t moves;
    gen_legal<c>(pos, moves);
    order_moves(pos, moves);


    if (moves.empty()) {
        if (pos.checkers(c) != bb::empty) {
            return -(MATE_SCORE + depth);
        }
        return 0;
    }

    if (pos.is_draw())
    {
        return 0;
    }

    if (depth == 0) {
        return evaluate_position<c>(pos);
    }

    int bestEval = -INFINITE;
    move_t best_move = move_t::null();

    for (const auto m : moves) {
        pos.do_move(m);
        int eval = -minimax<opponent>(pos, depth - 1, -beta, -alpha, searched, tt_hits);
        pos.undo_move(m);

        if (eval > bestEval)
        {
            best_move = m;
        }
        bestEval = std::max(bestEval, eval);
        alpha = std::max(alpha, eval);
        searched++;

        if (alpha >= beta) {
            break;
        }
    }

    g_tt.store({pos.hash(), depth, bestEval});

    return bestEval;
}



template <color_t c>
move_t find_best_move(position_t& pos, int depth) {
    move_list_t moves;
    gen_legal<c>(pos, moves);
    order_moves(pos, moves);


    move_t best_move = move_t::none();
    int best_eval = -INT32_MAX;
    int alpha = -INT32_MAX;
    int beta = INT32_MAX;
    int searched = 0;
    int tt_hits = 0;

    for (int i = 0; i < moves.size(); ++i) {
        searched ++;
        const auto m = moves[i];
        pos.do_move(m);
        int eval = -minimax<~c>(pos, depth - 1, -beta, -alpha, searched, tt_hits);
        pos.undo_move(m);

        if (eval > best_eval) {
            best_eval = eval;
            best_move = m;
        }

        alpha = std::max(alpha, eval);
    }
    std::cout << best_eval << std::endl;
    std::cout << searched << std::endl;
    std::cout << tt_hits << std::endl;


    return best_move;
}



#endif //SEARCH_H
