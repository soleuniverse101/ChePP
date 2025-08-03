//
// Created by paul on 7/29/25.
//

#ifndef SEARCH_H
#define SEARCH_H

#include "eval.h"
#include "move_ordering.h"
#include "tt.h"

#include <src/tbprobe.h>

template <color_t c>
int qsearch(position_t& pos, int alpha, int beta)
{
    int stand_pat = evaluate_position<c>(pos);
    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    move_list_t list;
    gen_tactical<c>(pos, list);
    //order_moves(pos, list);

    for (size_t i = 0; i < list.size(); ++i)
    {
        move_t m = list[i];

        pos.do_move(m);
        int score = -qsearch<!c>(pos, -beta, -alpha);
        pos.undo_move(m);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}


template <color_t c>
int minimax(position_t& pos, int depth, int alpha, int beta, int& searched, int& tt_hits) {
    constexpr auto opponent = ~c;



    move_list_t moves;

    gen_legal<c>(pos, moves);

    order_moves(pos, moves);

    /**
    if (const unsigned res = pos.wdl_probe(); res != TB_RESULT_FAILED)
    {
        if (res == TB_WIN)
        {
            return MATE_SCORE + depth;
        }
        if (res == TB_LOSS)
        {
            return -(MATE_SCORE + depth);
        }
        else
        {
            return 0;
        }
    } **/

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

    if (const auto entry = g_tt.probe(pos.hash(), depth, alpha, beta))
    {
        tt_hits++;
        return entry.value().m_score;
    }

    if (depth == 0) {
        //return evaluate_position<c>(pos);
        return qsearch<c>(pos, alpha, beta);
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

    g_tt.store(pos.hash(), depth, bestEval, alpha, beta);

    return bestEval;
}





template <color_t c>
move_t find_best_move(position_t& pos, int max_depth) {
    move_t best_move = move_t::none();

    int total_nodes = 0;
    int total_tt_hits = 0;


    if (const unsigned tb = pos.dtz_probe(); tb != TB_RESULT_FAILED)
    {
        if (TB_GET_PROMOTES(tb))
        {
            best_move = move_t::make<PROMOTION>(static_cast<square_t>(TB_GET_FROM(tb)), static_cast<square_t>(TB_GET_TO(tb)), QUEEN);
        } else if (TB_GET_EP(tb))
        {
            best_move = move_t::make<EN_PASSANT>(static_cast<square_t>(TB_GET_FROM(tb)), static_cast<square_t>(TB_GET_TO(tb)));
        }
        else
        {
            best_move = move_t::make<NORMAL>(static_cast<square_t>(TB_GET_FROM(tb)), static_cast<square_t>(TB_GET_TO(tb)));
        }
        return best_move;
    }

    for (int depth = 1; depth <= max_depth; ++depth) {
        move_list_t moves;
        gen_legal<c>(pos, moves);
        order_moves(pos, moves, best_move);

        int best_eval = -INT32_MAX;
        int alpha = -INT32_MAX;
        int beta = INT32_MAX;

        int nodes = 0;
        int tt_hits = 0;

        move_t current_best = move_t::none();


        for (int i = 0; i < moves.size(); ++i) {
            const auto m = moves[i];

            pos.do_move(m);

            int eval = -minimax<~c>(pos, depth - 1, -beta, -alpha, nodes, tt_hits);

            pos.undo_move(m);

            if (eval > best_eval) {
                best_eval = eval;
                current_best = m;
            }

            alpha = std::max(alpha, eval);
        }

        if (current_best != move_t::none()) {
            best_move = current_best;
        }

        total_nodes += nodes;
        total_tt_hits += tt_hits;

        std::cout << "Depth: " << depth << ", Eval: " << best_eval
                  << ", Nodes: " << nodes
                  << ", TT hits: " << tt_hits
                  << ", Hit Rate: " << static_cast<float>(tt_hits) / nodes * 100.0f << " %\n";
    }

    std::cout << "Total Nodes: " << total_nodes << "\n";
    std::cout << "Total TT Hits: " << total_tt_hits << "\n";
    std::cout << "Overall Hit Rate: " << static_cast<float>(total_tt_hits) / total_nodes * 100.0f << " %\n";

    return best_move;
}



#endif //SEARCH_H
