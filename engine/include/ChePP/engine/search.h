#ifndef SEARCHER_H
#define SEARCHER_H
#include "eval.h"
#include "move_ordering.h"
#include "nnue.h"
#include "tt.h"
#include <array>
#include <chrono>
#include <iostream>
#include <ranges>
#include <src/tbprobe.h>
#include <utility>

constexpr int INFINITE_SCORE = 32000;
struct SearchThread
{
    int                                   timeLimitMs = -1;
    bool                                  stop        = false;
    int64_t                               nodes       = 0;
    int64_t                               ttHits      = 0;
    std::chrono::steady_clock::time_point startTime;
    void                                  startTimer() { startTime = std::chrono::steady_clock::now(); }
    [[nodiscard]] bool                    timeUp() const
    {
        if (timeLimitMs <= 0)
            return false;
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        return elapsed >= timeLimitMs;
    }
};
struct SearchStack
{
    std::array<move_t, 2> killers   = {move_t::none(), move_t::none()};
    move_t                currentPV = move_t::none();
    int                   ply       = 0;
    bool                  didNull   = false;
};
template <color_t C>
class Searcher
{
  public:
    using Score = int;
    Searcher(const position_t& rootPos, Koi::Net& nnueNet, const int maxDepth, const int timeLimitMs = -1)
        : RootPos(rootPos), NNUE(nnueNet), MaxDepth(maxDepth)
    {
        Thread.timeLimitMs = timeLimitMs;
        HistoryScores.fill({});
        NNUE.accumulator().refresh<WHITE>(RootPos);
        NNUE.accumulator().refresh<BLACK>(RootPos);
        PVLength.fill(0);
    }
    move_t FindBestMove()
    {
        Thread.stop   = false;
        Thread.nodes  = 0;
        Thread.ttHits = 0;
        Thread.startTimer();
        BestMove = move_t::none();
        move_list_t rootMoves;
        gen_legal<C>(RootPos, rootMoves);
        g_tt.new_generation();
        Score prevEval = 0;
        for (int depth = 1; depth <= MaxDepth && !Thread.stop; ++depth)
        {
            constexpr int delta       = 25;
            int           alpha       = prevEval - delta;
            const int     beta        = prevEval + delta;
            move_t        currentBest = move_t::none();
            Score         bestEval    = -INFINITE_SCORE;
            order_moves(RootPos, rootMoves, BestMove, HistoryScores, RootStack[0].killers);
            PVLength[0] = 0;
            for (auto m : rootMoves)
            {
                if (Thread.timeUp())
                {
                    Thread.stop = true;
                    goto out_of_time;
                    ;
                }
                RootPos.do_move(m);
                NNUE.do_move<C>(RootPos, m);
                const Score eval = -Negamax<~C>(depth - 1, -beta, -alpha, 1);
                RootPos.undo_move(m);
                NNUE.undo_move();
                if (eval > bestEval)
                {
                    bestEval      = eval;
                    currentBest   = m;
                    PVTable[0][0] = m;
                    int childLen  = (1 < MAX_PLY) ? PVLength[1] : 0;
                    for (int i = 0; i < childLen; ++i)
                        PVTable[0][i + 1] = PVTable[1][i];
                    PVLength[0] = childLen + 1;
                }
                if (eval > alpha)
                    alpha = eval;
            }
            if (!Thread.stop && (bestEval <= prevEval - delta || bestEval >= prevEval + delta))
            {
                alpha                  = -INFINITE_SCORE;
                constexpr int betaFull = INFINITE_SCORE;
                bestEval               = -INFINITE_SCORE;
                PVLength[0]            = 0;
                for (auto m : rootMoves)
                {
                    if (Thread.timeUp())
                    {
                        Thread.stop = true;
                        goto out_of_time;
                        ;
                    }
                    RootPos.do_move(m);
                    NNUE.do_move<C>(RootPos, m);
                    Score eval = -Negamax<~C>(depth - 1, -betaFull, -alpha, 1);
                    RootPos.undo_move(m);
                    NNUE.undo_move();
                    if (eval > bestEval)
                    {
                        bestEval      = eval;
                        currentBest   = m;
                        PVTable[0][0] = m;
                        int childLen  = (1 < MAX_PLY) ? PVLength[1] : 0;
                        for (int i = 0; i < childLen; ++i)
                            PVTable[0][i + 1] = PVTable[1][i];
                        PVLength[0] = childLen + 1;
                    }
                    if (eval > alpha)
                        alpha = eval;
                }
            }
            if (currentBest != move_t::none())
                BestMove = currentBest;
            prevEval = bestEval;
            std::cout << "Depth: " << depth << ", Eval: " << bestEval << ", Nodes: " << Thread.nodes
                      << ", TT Hits: " << Thread.ttHits << ", Hit Rate: " << (100.0f * Thread.ttHits / Thread.nodes)
                      << " %, PV: ";
            for (int i = 0; i < PVLength[0]; ++i)
            {
                std::cout << PVTable[0][i] << " ";
            }
            std::cout << "\n";
        }
    out_of_time:
        std::cout << "Total Nodes: " << Thread.nodes << ", Total TT Hits: " << Thread.ttHits
                  << ", Overall Hit Rate: " << (100.0f * Thread.ttHits / Thread.nodes) << " %\n";
        return BestMove;
    }

  private:
    position_t                                       RootPos;
    Koi::Net&                                        NNUE;
    int                                              MaxDepth;
    move_t                                           BestMove;
    SearchThread                                     Thread;
    std::array<SearchStack, MAX_PLY>                 RootStack;
    enum_array<square_t, enum_array<square_t, int>>  HistoryScores{};
    std::array<std::array<move_t, MAX_PLY>, MAX_PLY> PVTable{};
    std::array<int, MAX_PLY>                         PVLength{};
    static inline bool                               isQuiet(const position_t& pos, const move_t m)
    {
        const bool givesCheck = (bb::attacks(pos.piece_type_at(m.from_sq()), m.to_sq(), pos.occupancy(), pos.color()) &
                                 bitboard_t::square(pos.ksq(~pos.color()))) != bb::empty();
        return m.type_of() != PROMOTION && !pos.is_occupied(m.to_sq()) && !givesCheck;
    }
    inline void pushKiller(SearchStack& ss, const move_t m)
    {
        if (ss.killers[0] == m)
            return;
        ss.killers[1] = ss.killers[0];
        ss.killers[0] = m;
    }
    template <color_t Side>
    Score QSearch(int alpha, int beta, int ply)
    {
        Thread.nodes++;
        constexpr auto Opponent = ~Side;
        PVLength[ply]           = 0;
        DenseBuffer<int32_t, 1> out;
        NNUE.forward(Side, out);
        Score standPat = out[0];
        if (standPat >= beta)
        {
            return beta;
        }
        if (standPat > alpha)
            alpha = standPat;
        move_list_t moves;
        gen_tactical<Side>(RootPos, moves);
        order_moves(RootPos, moves, RootStack[ply].currentPV, HistoryScores, RootStack[ply].killers);
        Score bestEval = standPat;
        for (auto m : moves)
        {
            RootPos.do_move(m);
            NNUE.do_move<Side>(RootPos, m);
            Score eval = -QSearch<Opponent>(-beta, -alpha, ply + 1);
            NNUE.undo_move();
            RootPos.undo_move(m);
            if (eval > bestEval)
            {
                bestEval        = eval;
                PVTable[ply][0] = m;
                int childLen    = PVLength[ply + 1];
                for (int i = 0; i < childLen; ++i)
                    PVTable[ply][i + 1] = PVTable[ply + 1][i];
                PVLength[ply] = childLen + 1;
            }
            if (bestEval > alpha)
                alpha = bestEval;
            if (alpha >= beta)
            {
                HistoryScores.at(m.from_sq()).at(m.to_sq()) += (ply + 1) * (ply + 1);
                break;
            }
        }
        return bestEval;
    }
    template <color_t Side>
    Score Negamax(int depth, int alpha, int beta, int ply)
    {
        if (Thread.timeUp())
        {
            Thread.stop = true;
            return 0;
        }
        Thread.nodes++;
        constexpr auto Opponent = ~Side;
        PVLength[ply]           = 0;
        move_list_t moves;
        gen_legal<Side>(RootPos, moves);
        if (moves.empty())
        {
            if (RootPos.checkers(Side) != bb::empty())
                return -(MATE_SCORE + depth);
            return 0;
        }
        if (RootPos.is_draw())
            return 0;
        const int alphaOrig = alpha;
        if (const auto entry = g_tt.probe(RootPos.hash(), depth, alpha, beta))
        {
            Thread.ttHits++;
            return entry.value().m_score;
        }
        if (depth >= 3 && !RootStack[ply].didNull && RootPos.checkers(Side) == bb::empty() &&
            (RootPos.occupancy().popcount() > 15))
        {
            constexpr int R = 2;
            RootPos.do_null_move();
            RootStack[ply + 1].didNull = true;
            Score nullEval             = -Negamax<~Side>(depth - R - 1, -beta, -beta + 1, ply + 1);
            RootPos.undo_null_move();
            RootStack[ply + 1].didNull = false;
            if (nullEval >= beta)
                return beta;
        }
        if (depth == 0)
            return QSearch<Side>(alpha, beta, ply);
        if (RootPos.checkers(~Side))
            depth++;
        Score  bestEval  = -INFINITE_SCORE;
        move_t bestMove  = move_t::none();
        bool   firstMove = true;
        order_moves(RootPos, moves, RootStack[ply].currentPV, HistoryScores, RootStack[ply].killers);
        for (auto m : moves)
        {
            RootPos.do_move(m);
            NNUE.do_move<Side>(RootPos, m);
            Score eval;
            if (firstMove)
            {
                eval      = -Negamax<Opponent>(depth - 1, -beta, -alpha, ply + 1);
                firstMove = false;
            }
            else
            {
                eval = -Negamax<Opponent>(depth - 1, -alpha - 1, -alpha, ply + 1);
                if (eval > alpha && eval < beta)
                    eval = -Negamax<Opponent>(depth - 1, -beta, -alpha, ply + 1);
            }
            NNUE.undo_move();
            RootPos.undo_move(m);
            if (eval > bestEval)
            {
                bestEval        = eval;
                bestMove        = m;
                PVTable[ply][0] = m;
                int childLen    = PVLength[ply + 1];
                for (int i = 0; i < childLen; ++i)
                    PVTable[ply][i + 1] = PVTable[ply + 1][i];
                PVLength[ply] = childLen + 1;
            }
            if (eval > alpha)
                alpha = eval;
            if (alpha >= beta)
            {
                HistoryScores.at(m.from_sq()).at(m.to_sq()) += depth * depth;
                if (isQuiet(RootPos, m))
                    pushKiller(RootStack[ply], m);
                break;
            }
        }
        g_tt.store(RootPos.hash(), depth, bestEval, alphaOrig, beta);
        return bestEval;
    }
};
#endif // SEARCHER_H