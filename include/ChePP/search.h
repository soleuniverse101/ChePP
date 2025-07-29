//
// Created by paul on 7/29/25.
//

#ifndef SEARCH_H
#define SEARCH_H

#include "eval.h"
#include "move_ordering.h"

constexpr int MATE_SCORE = 100000;
constexpr int INFINITE = 1000000;

template <color_t c>
int minimax(position_t& pos, int depth, int alpha, int beta) {
    constexpr auto opponent = ~c;

    move_list_t moves;
    gen_legal<c>(pos, moves);
    order_moves(pos, moves);

    if (moves.size() == 0) {


        if (pos.checkers(c) != bb::empty) {
            return -(MATE_SCORE + depth);
        }
        return 0;
    }

    if (depth == 0) {
        return evaluate_position<c>(pos);
    }

    int bestEval = -INFINITE;

    for (const auto m : moves) {
        pos.do_move(m);
        int eval = -minimax<opponent>(pos, depth - 1, -beta, -alpha);
        pos.undo_move(m);

        bestEval = std::max(bestEval, eval);
        alpha = std::max(alpha, eval);

        if (alpha >= beta) {
            break;
        }
    }

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

    for (int i = 0; i < moves.size(); ++i) {
        const auto m = moves[i];
        pos.do_move(m);
        int eval = -minimax<~c>(pos, depth - 1, -beta, -alpha);
        pos.undo_move(m);

        if (eval > best_eval) {
            best_eval = eval;
            best_move = m;
        }

        alpha = std::max(alpha, eval);
    }
    std::cout << best_eval << std::endl;

    return best_move;
}



#endif //SEARCH_H
