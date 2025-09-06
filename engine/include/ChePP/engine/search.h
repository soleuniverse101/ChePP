#ifndef SEARCHER_H
#define SEARCHER_H

#include "move_ordering.h"
#include "nnue.h"
#include "tt.h"

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

struct SearchInfo {
    int depth;
    int max_time_ms;
    std::chrono::steady_clock::time_point startTime{};
    uint64_t nodes{};
    bool stop{false};

    SearchInfo(const int depth, const int max_time) : depth(depth), max_time_ms(max_time) {}
};

struct SearchThread {
    explicit SearchThread(SearchInfo* info, Engine&& engine)
        : m_info(info), m_engine(engine) {}

    SearchInfo* m_info;
    Engine m_engine;
    Move bestMove{};
    std::array<SearchStackNode, MAX_PLY> m_ss;

    [[nodiscard]] bool timeUp() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - m_info->startTime)
                   .count() > m_info->max_time_ms;
    }

    void IterativeDeepening();
    int AspirationWindow(int depth, int prev_eval);
    int Negamax(int depth, int alpha, int beta, int ss_idx);
    int QSearch(int alpha, int beta, int ss_idx);


};

inline constexpr std::array<std::array<int, 256>, MAX_PLY> lmr_table = [] () {
    std::array<std::array<int, 256>, MAX_PLY> lmr{};
    for (int d = 1; d < MAX_PLY; ++d) {
        for (int m = 1; m < 256; ++m) {
            lmr[d][m] = static_cast<int>(std::log(d + 1) * std::log(m + 1) / 2);
        }
    }
    return lmr;
} ();


inline void SearchThread::IterativeDeepening() {

    int prev_eval = 0;
    for (int depth = 1; depth <= m_info->depth && !m_info->stop; ++depth) {
        const int eval = AspirationWindow(depth, prev_eval);
        if (!m_info->stop)
        {
            prev_eval = eval;

            std::string score;
            if (eval >= MATE_IN_MAX_PLY)
            {
                score.append("mate in ");
                score.append(std::to_string(MATE  - eval));
            }
            else score = std::to_string(eval);

            std::cout << "Depth " << depth
                      << " Eval " << score
                      << " Nodes " << m_info->nodes
                      << " BestMove " << bestMove
                      << std::endl;
        }

    }
}


inline int SearchThread::AspirationWindow(const int depth, const int prev_eval) {
    const int window = 50 - depth;
    const int alpha = prev_eval - window;
    const int beta  = prev_eval + window;

    int eval = Negamax(depth, alpha, beta, 0);

    if (eval <= alpha || eval >= beta) {
        eval = Negamax(depth, -INF_SCORE, INF_SCORE, 0);
    }
    return eval;
}

inline auto adjust_tt_score(const int score, const int ply) {
    if (score >= MATE_IN_MAX_PLY) return score - ply;
    if (score <= -MATE_IN_MAX_PLY) return score + ply;
    return score;
};



inline int SearchThread::Negamax(int depth, int alpha, int beta, const int ss_idx) {
    if (timeUp()) { m_info->stop = true; return 0; }
    if (depth <= 0) return QSearch(alpha, beta, ss_idx);

    m_info->nodes++;
    const Position& pos = m_engine.position;
    SearchStackNode& ss = m_ss[ss_idx];
    SearchStackNode& ss_next = m_ss[ss_idx + 1];
    SearchStackNode& ss_prev = m_ss[ss_idx - 1];


    const int alpha_org = alpha;
    const bool is_root = ss_idx == 0;
    const bool in_check = pos.checkers(pos.color()).value();

    if (!is_root) {
        if (pos.is_draw()) return 0;
        if (ss.ply >= MAX_PLY) return m_engine.evaluate();

        // this speeds up mate cases
        // our worse move is to be mated on the spot
        alpha = std::max(alpha, mated_in(ss.ply));
        // their best move is to mate next turn
        beta = std::min(beta, mate_in(ss.ply + 1));

        if (alpha >= beta) return alpha;
    }

    const bool is_pv = beta - alpha > 1;

    depth += in_check;

    const auto tt_hit = g_tt.probe(pos.hash());
    if (!is_pv && tt_hit) {
        const tt_entry_t& e = *tt_hit;
        if (e.m_depth >= depth) {
            const int score = adjust_tt_score(e.m_score, ss.ply);
            if (e.m_bound == EXACT) return score;
            if (e.m_bound == LOWER && score >= alpha) return score;
            if (e.m_bound == UPPER && score <= beta) return score;
        }
    }

    const int static_eval = tt_hit ? tt_hit->m_score : m_engine.evaluate();

    MoveList moves = gen_legal(pos);

    if (moves.empty()) {
        if (in_check) return mated_in(ss.ply);
        return 0;
    }

    score_moves(pos, moves, tt_hit ? tt_hit->m_move : Move::none(), ss);
    moves.sort();

    // null move pruning
    // idea : if we expect we will beat beta, we offer a free move and search at reduced depth
    // if eval comes from tt, is upper bounded and not higher that beta, we cant assume anything on score
    // evaluating is not worth it so we just skip
    // only do it if there are enough pieces to not avoid zugzwang blindness
    if (!is_root && !is_pv && ss_prev.move != Move::null() && !in_check && depth >= 3 && static_eval >= beta &&
         !(tt_hit && tt_hit->m_bound == UPPER && tt_hit->m_score <= beta) && pos.pieces_bb(KNIGHT, BISHOP, ROOK, QUEEN).popcount() >= 3)
    {
        const int reduction = 3 + depth / 3 + (static_eval - beta) / 100;
        m_engine.position.do_move(Move::null());
        ss.move = Move::null();
        ss_next.ply = ss.ply + 1;

        int score = -Negamax(depth - 1 - reduction, -beta, -beta + 1, ss_idx + 1);

        ss.move = Move::none();
        m_engine.position.undo_move();

        if (score >= beta)
        {
            if (std::abs(score) >= MATE_IN_MAX_PLY) score = beta;
            return score;
        }
    }


    if (!is_root && !is_pv && !in_check && depth >= 3)
    {
        int prob_beta = beta + 150;
        const int reduction = 3;
        MoveList tactical = filter_tactical(pos, gen_legal(pos));
        score_moves(pos, tactical, tt_hit ? tt_hit->m_move : Move::none(), ss);
        tactical.sort();

        for (auto [m, s] : tactical) {
            if (m == tt_hit->m_move || s < -1000)
            {
                continue;
            }
            m_engine.do_move(m);
            ss.move = m;
            ss_next.ply = ss.ply + 1;


            int score = -QSearch(-prob_beta, -prob_beta + 1, ss_idx + 1);
            if (score >= prob_beta)
            {
                prob_beta = -Negamax(depth - 1 - reduction, -beta, -beta + 1, ss_idx + 1);
            }


            ss.move = Move::none();
            m_engine.undo_move();

            if (score >= prob_beta) {
                return score;
            }
        }
    }

    int best_eval = -INF_SCORE;
    Move local_best = Move::none();
    bool first_move = true;
    int move_idx = 0;

    for (auto [m, s] : moves) {
        m_engine.do_move(m);
        ss.move = m;
        ss_next.ply = ss.ply + 1;

        bool is_quiet = !pos.taken() && m.type_of() != PROMOTION;
        int score;

        int search_depth = depth;

        if (!is_root && !is_pv && depth >= 3 && !in_check && move_idx > 0) {
            int reduction = std::min(lmr_table[depth][move_idx], depth - 1);
            reduction += is_quiet;
            reduction -= s / 4000;
            reduction = std::min(4, reduction);
            search_depth -= reduction;
            search_depth = std::max(1, search_depth);
        }

        if (first_move || in_check) {
            score = -Negamax(search_depth - 1, -beta, -alpha, ss_idx + 1);
        } else {
            score = -Negamax(search_depth - 1, -alpha - 1, -alpha, ss_idx + 1);
            if (score > alpha && score < beta) {
                score = -Negamax(depth - 1, -beta, -alpha, ss_idx + 1);
            }
        }

        ss.move = Move::none();
        m_engine.undo_move();

        if (score > best_eval) {
            best_eval = score;
            local_best = m;
        }
        if (score > alpha) alpha = score;

        if (alpha >= beta) {
            if (is_quiet) {
                if (ss.killer1 != m) {
                    ss.killer2 = ss.killer1;
                    ss.killer1 = m;
                }
                ss.history[m.from_sq()][m.to_sq()] += depth * depth;
            }
            break;
        }

        first_move = false;
        move_idx++;
    }

    bool best_valid = !m_info->stop && local_best != Move::none();
    if (is_root && best_valid) bestMove = local_best;

    tt_bound_t bound;
    if (best_eval <= alpha_org) bound = UPPER;
    else if (best_eval >= beta) bound = LOWER;
    else bound = EXACT;

    if (best_valid) g_tt.store(pos.hash(), depth, adjust_tt_score(best_eval, ss.ply), bound, local_best);

    return best_eval;
}


inline int SearchThread::QSearch(int alpha, int beta, const int ss_idx) {
    m_info->nodes++;

    const Position& pos = m_engine.position;
    SearchStackNode& ss = m_ss[ss_idx];
    SearchStackNode& ss_next = m_ss[ss_idx + 1];

    if (pos.is_draw()) return 0;

    const auto tt_hit = g_tt.probe(pos.hash());
    if (tt_hit) {
        const tt_entry_t& e = *tt_hit;
        const int score = adjust_tt_score(e.m_score, ss.ply);
        if (e.m_bound == EXACT) return score;
        if (e.m_bound == LOWER && score >= alpha) return score;
        if (e.m_bound == UPPER && score <= beta) return score;
    }


    const int stand_pat = m_engine.evaluate();
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    const MoveList moves = gen_legal(m_engine.position);
    MoveList tactical = filter_tactical(m_engine.position, moves);
    score_moves(pos, tactical, tt_hit ? tt_hit->m_move : Move::none(), m_ss[ss_idx]);
    tactical.sort();

    int best_eval = stand_pat;
    for (auto [m, s] : tactical) {
        if (pos.is_occupied(m.to_sq()) && s < -1000) // see pruning on captures
        {
            continue;
        }

        m_engine.do_move(m);
        ss.move = m;
        ss_next.ply = ss.ply + 1;



        const int score = -QSearch(-beta, -alpha, ss_idx + 1);

        ss.move = Move::none();

        m_engine.undo_move();

        if (score > best_eval) best_eval = score;
        if (best_eval > alpha) alpha = best_eval;
        if (alpha >= beta) break;
    }
    return best_eval;
}


struct SearchThreadHandler {
    std::vector<std::unique_ptr<SearchThread>> threads;
    std::vector<std::thread> workers;
    SearchInfo m_info;

    explicit SearchThreadHandler(const size_t numThreads, const SearchInfo& info, const Position& pos)
        : m_info(info) {
        threads.reserve(numThreads);
        for (size_t i = 0; i < numThreads; i++) {
            threads.push_back(std::make_unique<SearchThread>(&m_info, Engine(pos)));
        }
    }

    void start() {
        m_info.startTime = std::chrono::steady_clock::now();
        m_info.nodes     = 0;
        m_info.stop      = false;

        workers.clear();
        workers.reserve(threads.size());

        for (int i = 0; i < threads.size(); i++) {
            workers.emplace_back([this, i]() {
                threads.at(i)->IterativeDeepening();
            });
        }
    }

    void join() {
        for (auto& w : workers)
            if (w.joinable()) w.join();
    }

    Move get_best_move() const
    {
        std::unordered_map<uint16_t, int> moves;
        for (const auto& t : threads)
        {
            if (moves.contains(t.get()->bestMove.raw()))
            {
                moves.at(t.get()->bestMove.raw())++;
            } else
            {
                moves.emplace(t.get()->bestMove.raw(), 1);
            }
        }
        Move best;
        int votes = 0;
        for (const auto entry : moves)
        {
            if (entry.second > votes)
            {
                votes = entry.second;
                best = Move{entry.first};
            }
        }
        return best;
    }

    void stop_all() { m_info.stop = true; }
};

#endif // SEARCHER_H
