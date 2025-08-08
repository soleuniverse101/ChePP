#include "ChePP/engine/bitboard.h"
#include "ChePP/engine/movegen.h"
#include "ChePP/engine/position.h"
#include "ChePP/engine/search.h"
#include "ChePP/engine/tt.h"
#include "ChePP/engine/tb.h"
#include "ChePP/engine/nnue.h"
#include "ChePP/engine/zobrist.h"

#include <chrono>
#include <iostream>
#include <random>

#include <iostream>



void perft_divide(position_t& pos, int depth) {
    move_list_t l;
    if (pos.color() == WHITE)
        gen_legal<WHITE>(pos, l);
    else
        gen_legal<BLACK>(pos, l);

    size_t total = 0;

    for (size_t i = 0; i < l.size(); ++i) {
        const auto mv = l[i];
        pos.do_move(mv);

        size_t nodes = 0;
        perft(pos, depth - 1, nodes);

        pos.undo_move(mv);

        std::cout << pos.piece_at(mv.from_sq()) << " " << mv.from_sq() << " " << mv.to_sq() << ": " << nodes << '\n';
        total += nodes;
    }

    std::cout << "Total: " << total << '\n';
}

void init_random_weights(layer_t<int16_t, feature_t::n_features, nnue_t::s_accumulator_size>& layer, int16_t seed = 1) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int16_t> dist(1, 3);

    for (auto& row : layer.m_weights) {
        for (auto& w : row) {
            w = dist(rng);
        }
    }

    for (auto& b : layer.m_biases) {
        b = dist(rng);
    }
}

void print_accumulator(accumulator_t<nnue_t::s_accumulator_size>& acc, color_t c) {
    std::cout << (c == WHITE ? "WHITE accumulator:\n" : "BLACK accumulator:\n");
    for (size_t i = 0; i < nnue_t::s_accumulator_size; ++i) {
        std::cout << acc[c][i] << " ";
        if (i % 16 == 15) std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    nnue_t* net = new nnue_t();
    init_random_weights(net->m_accumulator_layer);

    zobrist_t::init(0xFADA);
    bb::init();
    std::cout << bb::pseudo_attack<KNIGHT>(C3, WHITE).to_string() << "\n";
    g_tt.init(64);
    tb_init("/home/paul/code/ChePP/scripts/syzygy");

    position_t pos;
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::cout << pos;

    size_t out;

    perft(pos, 1, out);

    std::cout << out << std::endl;

    move_list_t moves;
    gen_legal<WHITE>(pos, moves);
    for (size_t i = 0; i < moves.size(); ++i)
    {
        const auto mv = moves[i];
        std::cout << mv << std::endl;
    }


    net->refresh_accumulator(pos, WHITE);
    net->refresh_accumulator(pos, BLACK);

    std::cout << "After refresh:\n";
    print_accumulator(net->m_accumulator_stack[net->m_accumulator_idx], WHITE);
    print_accumulator(net->m_accumulator_stack[net->m_accumulator_idx], BLACK);

    constexpr move_t move = move_t::make<NORMAL>(E2, E4);
    pos.do_move(move);



    constexpr int iterations = 1'000'000;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            net->push_accumulator();
            net->add_dirty_move<WHITE>(pos, move);
            net->update_accumulator(pos, WHITE);
            net->update_accumulator(pos, BLACK);
            net->pop_accumulator();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto total_ns = duration_cast<std::chrono::nanoseconds>(end - start).count();
        double avg_ns = static_cast<double>(total_ns) / iterations;

        std::cout << "Average time per iteration: " << avg_ns << " ns\n";

    std::cout << "After move E2->E4 and update:\n";
    print_accumulator(net->m_accumulator_stack[net->m_accumulator_idx], WHITE);
    print_accumulator(net->m_accumulator_stack[net->m_accumulator_idx], BLACK);



    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");


    zobrist_t::init(0xFADA);
    bb::init();
    g_tt.init(64);
    tb_init("/home/paul/code/ChePP/scripts/syzygy");

    std::cout << TB_LARGEST << std::endl;



    //pos.from_fen("k7/8/B2NK3/8/8/8/8/8 w - - 0 1");
    //pos.from_fen("r1bq1rk1/pp3ppp/2n1pn2/2bp4/2B1P3/2N2N2/PPP2PPP/R1BQ1RK1 w - - 0 10");
    //pos.from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    pos.from_fen("8/8/p1b5/8/2k5/1pP2P2/3K4/3N4 w - - 0 1");
    std::cout << pos;



    for (int i = 0; i < 50; i++)
    {
        const auto mv = find_best_move<WHITE>(pos, 10);
        g_tt.new_generation();
        pos.do_move(mv);
        std::cout << pos;
        const auto mv2 = find_best_move<BLACK>(pos, 10);
        g_tt.new_generation();
        pos.do_move(mv2);
        std::cout << pos;
    }

}
