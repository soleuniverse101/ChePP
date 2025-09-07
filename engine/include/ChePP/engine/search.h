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
#include <utility>
#include <vector>

struct SearchInfo {
    int depth{};
    int max_time_ms{};
    std::chrono::steady_clock::time_point startTime{};
    uint64_t nodes{};
    bool stop{false};

    SearchInfo() = default;
    SearchInfo(const int depth, const int max_time) : depth(depth), max_time_ms(max_time) {}
};

struct SearchThread
{
    explicit SearchThread(const int id, SearchInfo* info, const Position& pos, std::span<Move> moves)
        : id(id), m_info(info)
    {
        m_positions.reserve(MAX_PLY + moves.size() + 1);
        m_accumulators.reserve(MAX_PLY + 1);
        m_ss = std::make_unique<SearchStackNode[]>(MAX_PLY);

        m_positions.emplace_back(pos);
        for (const auto m : moves)
        {
            m_positions.emplace_back(m_positions.back());
            m_positions.back().do_move(m);
        }
        search_start = m_positions.size();
        m_accumulators.emplace_back(m_positions.back());
    }

    int id;
    SearchInfo* m_info;

    int search_start;

    std::vector<Position> m_positions;
    std::vector<Accumulator> m_accumulators;
    std::unique_ptr<SearchStackNode[]> m_ss;

    Move bestMove;

    [[nodiscard]] bool timeUp() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - m_info->startTime)
                   .count() > m_info->max_time_ms;
    }

    int ply() const {
        return m_positions.size() - search_start;
    }

    template <bool UpdateNNUE = true>
    void do_move(const Move move)
    {
        m_ss[ply()].move = move;

        const Position& prev_pos = m_positions.back();
        m_positions.push_back(prev_pos);
        Position& next_pos = m_positions.back();
        next_pos.do_move(move);

        if constexpr (UpdateNNUE)
        {
            const Accumulator& prev_acc = m_accumulators.back();
            m_accumulators.emplace_back(prev_acc, next_pos, prev_pos);
        }

    }

    template <bool UpdateNNUE = true>
    void undo_move()
    {

        m_positions.pop_back();
        m_ss[ply()].move = Move::none();
        if constexpr (UpdateNNUE) m_accumulators.pop_back();
    }

    int32_t evaluate() {
        return m_accumulators.back().evaluate(m_positions.back().side_to_move());
    }

    bool is_repetition() {
        return Position::is_repetition(m_positions);
    }

    void IterativeDeepening();
    int AspirationWindow(int depth, int prev_eval);
    int Negamax(int depth, int alpha, int beta);
    int QSearch(int alpha, int beta);
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

inline constexpr int FUTILITY_DEPTH_MAX = 3;
inline constexpr int FUTILITY_BASE_MARGIN = 100;
inline constexpr int FUTILITY_DEPTH_SCALE = 120;

inline int futility_margin_for_depth(int depth) {
    depth = std::max(1, std::min(depth, MAX_PLY));
    return FUTILITY_BASE_MARGIN + FUTILITY_DEPTH_SCALE * depth;
}


inline void SearchThread::IterativeDeepening() {
    int prev_eval = evaluate();
    for (int depth = 1; depth <= m_info->depth && !m_info->stop; ++depth) {
        const int eval = AspirationWindow(depth, prev_eval);
        if (!m_info->stop)
        {
            prev_eval = eval;

            if (id == 0)
            {
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
                          << " best " << bestMove
                          << std::endl;
            }

        }
    }
    std::cout << "bestmove " << bestMove << std::endl;
}


inline int SearchThread::AspirationWindow(const int depth, const int prev_eval) {
    int window = 50 - depth;
    int alpha = prev_eval - window;
    int beta  = prev_eval + window;

    int eval = Negamax(depth, alpha, beta);

    while (eval <= alpha || eval >= beta) {
        window *= 2;
        alpha = eval - window;
        beta  = eval + window;

        eval = Negamax(depth, -INF_SCORE, INF_SCORE);
    }
    return eval;
}

inline auto store_tt_score(const int score, const int ply) {
    if (score >= MATE_IN_MAX_PLY) return score - ply;
    if (score <= -MATE_IN_MAX_PLY) return score + ply;
    return score;
};

inline auto read_tt_score(const int score, const int ply) {
    if (score >= MATE_IN_MAX_PLY) return score + ply;
    if (score <= -MATE_IN_MAX_PLY) return score - ply;
    return score;
};





inline int SearchThread::Negamax(int depth, int alpha, int beta) {
    if (timeUp()) { m_info->stop = true; return 0; }
    if (depth <= 0) return QSearch(alpha, beta);

    m_info->nodes++;
    Position& pos = m_positions.back();
    SearchStackNode& ss = m_ss[ply()];


    const int alpha_org = alpha;
    const bool is_root = ply() == 0;
    const bool in_check = pos.checkers(pos.side_to_move()).value();

    if (!is_root) {
        if (is_repetition()) return 0;
        if (ply() >= MAX_PLY) return evaluate();

        // this speeds up mate cases
        // our worse move is to be mated on the spot
        alpha = std::max(alpha, mated_in(ply()));
        // their best move is to mate next turn
        beta = std::min(beta, mate_in(ply() + 1));

        if (alpha >= beta) return alpha;
    }

    const bool is_pv = beta - alpha > 1;

    depth += in_check;

    const auto tt_hit = g_tt.probe(pos.hash());
    if (!is_pv && tt_hit) {
        const tt_entry_t& e = *tt_hit;
        if (e.m_depth >= depth) {
            const int score = read_tt_score(e.m_score, ply());
            if (e.m_bound == EXACT) return score;
            if (e.m_bound == LOWER && score >= alpha) return score;
            if (e.m_bound == UPPER && score <= beta) return score;
        }
    }

    const int static_eval = tt_hit ? tt_hit->m_score : evaluate();

    MoveList moves = gen_legal(pos);

    if (moves.empty()) {
        if (in_check) return mated_in(ply());
        return 0;
    }

    score_moves(pos, moves, tt_hit ? tt_hit->m_move : Move::none(), ss);
    moves.sort();

    // null move pruning
    // idea : if we expect we will beat beta, we offer a free move and search at reduced depth
    // if eval comes from tt, is upper bounded and not higher that beta, we cant assume anything on score
    // evaluating is not worth it so we just skip
    // only do it if there are enough pieces to not avoid zugzwang blindness
    if (!is_root && !is_pv && m_ss[ply() - 1].move != Move::null() && !in_check && depth >= 3 && static_eval >= beta &&
         !(tt_hit && tt_hit->m_bound == UPPER && tt_hit->m_score < beta) &&
         std::abs(static_eval) < MATE_IN_MAX_PLY &&
         pos.occupancy(KNIGHT, BISHOP, ROOK, QUEEN).popcount() >= 3)
    {
        const int reduction = 3 + depth / 3 + std::clamp((static_eval - beta) / 100 ,-4, 4);
        do_move<false>(Move::null());

        int score = -Negamax(depth - 1 - reduction, -beta, -beta + 1);

        undo_move<false>();

        if (score >= beta)
        {
            if (std::abs(score) >= MATE_IN_MAX_PLY)
            {
                score = beta;
            }

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
            do_move(m);


            int score = -QSearch(-prob_beta, -prob_beta + 1);
            if (score >= prob_beta)
            {
                prob_beta = -Negamax(depth - 1 - reduction, -beta, -beta + 1);
            }

            undo_move();

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

        bool is_quiet = !pos.piece_at(m.to_sq()) && m.type_of() != EN_PASSANT && m.type_of() != PROMOTION;
        //if (is_root) { std::cout << m << std::endl; }

        if (false && !is_root && !is_pv && !in_check && !first_move && is_quiet && depth <= FUTILITY_DEPTH_MAX) {
            const int margin = futility_margin_for_depth(depth);
            if (static_eval + margin <= alpha) {
                move_idx++;
                first_move = false;
                continue;
            }
        }

        if (false && !is_root && !is_pv && !in_check && !first_move && is_quiet && depth <= 3) {
            const int lmp_limit = 3 + depth * depth;
            if (move_idx >= lmp_limit) {
                move_idx++;
                first_move = false;
                continue;
            }
        }

        do_move(m);

        int score;

        int search_depth = depth;

        if (!is_root && !is_pv && depth >= 3 && !in_check && move_idx > 0) {
            int reduction = std::min(lmr_table[depth][move_idx], depth - 1);
            reduction += is_quiet;
            //reduction -= s / 4000;
            reduction = std::clamp(reduction, 0, depth - 1);
            search_depth -= reduction;
        }

        if (first_move || in_check) {
            score = -Negamax(search_depth - 1, -beta, -alpha);
        } else {
            score = -Negamax(search_depth - 1, -alpha - 1, -alpha);
            if (score > alpha && score < beta) {
                score = -Negamax(depth - 1, -beta, -alpha);
            }
        }

        undo_move();

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

    if (best_valid) g_tt.store(pos.hash(), depth, store_tt_score(best_eval, ply()), bound, local_best);

    return best_eval;
}


inline int SearchThread::QSearch(int alpha, int beta) {
    m_info->nodes++;

    const Position& pos = m_positions.back();
    SearchStackNode& ss = m_ss[ply()];

    if (ply() >= MAX_PLY) return evaluate();

    if (is_repetition()) return 0;

    const MoveList moves = gen_legal(pos);
    if (moves.empty())
    {
        if (pos.checkers(pos.side_to_move()))
        {
            return mated_in(ply());
        }
        return 0;
    }

    const auto tt_hit = g_tt.probe(pos.hash());
    if (tt_hit) {
        const tt_entry_t& e = *tt_hit;
        const int score = read_tt_score(e.m_score, ply());
        if (e.m_bound == EXACT) return score;
        if (e.m_bound == LOWER && score >= alpha) return score;
        if (e.m_bound == UPPER && score <= beta) return score;
    }

    const int stand_pat = evaluate();
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;


    MoveList tactical = filter_tactical(pos, moves);
    score_moves(pos, tactical, tt_hit ? tt_hit->m_move : Move::none(), ss);
    tactical.sort();

    int best_eval = stand_pat;
    for (auto [m, s] : tactical) {
        if (pos.is_occupied(m.to_sq()) && s < -1000) // see pruning on captures
        {
            continue;
        }

        do_move(m);

        const int score = -QSearch(-beta, -alpha);

        ss.move = Move::none();

        undo_move();

        if (score > best_eval) best_eval = score;
        if (best_eval > alpha) alpha = best_eval;
        if (alpha >= beta) break;
    }
    return best_eval;
}






struct SearchThreadHandler {
    std::vector<std::unique_ptr<SearchThread>> threads{};
    std::vector<std::thread> workers{};
    SearchInfo m_info;

    SearchThreadHandler() = default;

    void set(const size_t numThreads, const SearchInfo& info, const Position& pos, const std::span<Move> moves)
    {
        m_info = info;
        threads.clear();
        threads.reserve(numThreads);
        for (size_t i = 0; i < numThreads; i++) {
            threads.push_back(std::make_unique<SearchThread>(i, &m_info, pos, moves));
        }
    }



    void start() {
        g_tt.new_generation();
        m_info.startTime = std::chrono::steady_clock::now();
        m_info.nodes     = 0;
        m_info.stop      = false;

        workers.clear();
        workers.reserve(threads.size());

        for (const auto & thread : threads) {
            workers.emplace_back([&thread]() {
                thread->IterativeDeepening();
            });
        }
    }

    void join() {
        for (auto& w : workers)
            if (w.joinable()) w.join();
    }

    [[nodiscard]] Move get_best_move() const {
        std::unordered_map<uint16_t, int> move_votes;

        for (const auto& t : threads) {
            move_votes[t->bestMove.raw()]++;
        }

        const auto it = std::ranges::max_element(move_votes,
            [](const auto& a, const auto& b) { return a.second < b.second; }
        );

        return it != move_votes.end() ? Move{it->first} : Move{};
    }

    void stop_all() { m_info.stop = true; }
};

#endif // SEARCHER_H
