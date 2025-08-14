#ifndef SEARCHER_H
#define SEARCHER_H

#include "tt.h"
#include "eval.h"
#include "move_ordering.h"
#include "nnue.h"
#include <src/tbprobe.h>
#include <array>
#include <iostream>
#include <ranges>
#include <utility>
#include <chrono>

template <color_t C>
class Searcher {
public:
    using Score = int;

    Searcher(const position_t& rootPos, Koi::Net& nnueNet, int maxDepth, int timeLimitMs = -1)
        : RootPos(rootPos), NNUE(nnueNet), MaxDepth(maxDepth), TimeLimitMs(timeLimitMs) {
        HistoryScores.fill({});
        NNUE.accumulator().refresh<WHITE>(RootPos);
        NNUE.accumulator().refresh<BLACK>(RootPos);
    }

    move_t FindBestMove() {
        BestMove = move_t::none();
        int totalNodes = 0;
        int totalTTHits = 0;

        move_list_t moves;
        gen_legal<C>(RootPos, moves);

        StartTime = std::chrono::steady_clock::now();

        for (int depth = 1; depth <= MaxDepth; ++depth) {
            move_t currentBest = move_t::none();
            if (TimeLimitMs > 0 && time_up()) break;

            int alpha = -INT32_MAX;
            int beta = INT32_MAX;
            int nodes = 0;
            int ttHits = 0;

            order_moves(RootPos, moves, BestMove, HistoryScores);

            Score bestEval = -INT32_MAX;
            for (auto m : moves) {
                //if (TimeLimitMs > 0 && time_up()) break;

                RootPos.do_move(m);
                NNUE.do_move<C>(RootPos, m);

                Score eval = -Negamax<~C>(depth - 1, -beta, -alpha, nodes, ttHits);

                RootPos.undo_move(m);
                NNUE.undo_move();

                if (eval > bestEval) {
                    bestEval = eval;
                    currentBest = m;
                }

                alpha = std::max(alpha, eval);
            }

            if (currentBest != move_t::none()) {
                BestMove = currentBest;
            }

            totalNodes += nodes;
            totalTTHits += ttHits;

            std::cout << "Depth: " << depth
                      << ", Eval: " << bestEval
                      << ", Nodes: " << nodes
                      << ", TT Hits: " << ttHits
                      << ", Hit Rate: " << static_cast<float>(ttHits) / nodes * 100.0f
                      << " %\n";
        }

        std::cout << "Total Nodes: " << totalNodes
                  << ", Total TT Hits: " << totalTTHits
                  << ", Overall Hit Rate: " << static_cast<float>(totalTTHits) / totalNodes * 100.0f
                  << " %\n";

        return BestMove;
    }

private:
    position_t RootPos;
    Koi::Net& NNUE;
    int MaxDepth;
    int TimeLimitMs;
    move_t BestMove;
    enum_array<square_t, enum_array<square_t, int>> HistoryScores{};
    std::chrono::steady_clock::time_point StartTime;

    bool time_up() const {
        if (TimeLimitMs <= 0) return false;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - StartTime)
                           .count();
        return elapsed >= TimeLimitMs;
    }

    template <color_t Side>
    Score QSearch(int alpha, int beta, int depth, int& searched) {
        constexpr auto Opponent = ~Side;
        searched++;

        DenseBuffer<int32_t, 1> out;
        NNUE.forward(Side, out);
        Score stand_pat = out[0];


        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) alpha = stand_pat;

        move_list_t moves;
        gen_tactical<Side>(RootPos, moves);
        order_moves(RootPos, moves, BestMove, HistoryScores);

        Score bestEval = stand_pat;
        for (auto m : moves) {
            RootPos.do_move(m);
            NNUE.do_move<Side>(RootPos, m);

            Score eval = -QSearch<Opponent>(-beta, -alpha, depth + 1, searched);

            NNUE.undo_move();
            RootPos.undo_move(m);

            if (eval > bestEval) bestEval = eval;
            if (bestEval > alpha) alpha = bestEval;

            if (alpha >= beta) {
                HistoryScores.at(m.from_sq()).at(m.to_sq()) += (depth + 1) * (depth + 1);
                break;
            }
        }
        return bestEval;
    }

    template <color_t Side>
    Score Negamax(int depth, int alpha, int beta, int& searched, int& ttHits) {

        constexpr auto Opponent = ~Side;
        move_list_t moves;
        gen_legal<Side>(RootPos, moves);
        order_moves(RootPos, moves, BestMove, HistoryScores);

        if (moves.empty()) {
            if (RootPos.checkers(Side) != bb::empty()) return -(MATE_SCORE + depth);
            return 0;
        }
        if (RootPos.is_draw()) return 0;

        if (const auto entry = g_tt.probe(RootPos.hash(), depth, alpha, beta)) {
            ttHits++;
            return entry.value().m_score;
        }

        if (depth == 0) return QSearch<Side>(alpha, beta, 0, searched);
        if (RootPos.checkers(~Side)) depth++;

        Score bestEval = -INFINITE;
        move_t bestMoveLocal = move_t::null();

        for (auto m : moves) {
            RootPos.do_move(m);
            NNUE.do_move<Side>(RootPos, m);

            Score eval = -Negamax<Opponent>(depth - 1, -beta, -alpha, searched, ttHits);

            NNUE.undo_move();
            RootPos.undo_move(m);

            if (eval > bestEval) {
                bestEval = eval;
                bestMoveLocal = m;
            }

            alpha = std::max(alpha, eval);
            searched++;

            if (alpha >= beta) {
                HistoryScores.at(m.from_sq()).at(m.to_sq()) += depth * depth;
                break;
            }
        }

        g_tt.store(RootPos.hash(), depth, bestEval, alpha, beta);
        return bestEval;
    }
};

#endif // SEARCHER_H
