
#include "ChePP/engine/bitboard.h"
#include "ChePP/engine/types.h"
#include "ChePP/engine/position.h"
#include "ChePP/engine/movegen.h"
#include "ChePP/engine/search.h"
#include <chrono>
#include <iostream>
#include <random>

#include <iostream>

#include "ChePP/engine/types.h"
#include "ChePP/engine/nnue.h"



void perft_divide(Position& pos, int depth) {
    MoveList l = gen_moves(pos);
    size_t total = 0;


    for (auto [mv, _] : l)
    {
        pos.do_move(mv);

        size_t nodes = 0;
        perft(pos, depth - 1, nodes);

        pos.undo_move();

        std::cout << pos.piece_at(mv.from_sq()) << " " << mv.from_sq() << " " << mv.to_sq() << ": " << nodes << '\n';
        total += nodes;
    }


    std::cout << "Total: " << total << '\n';
}


/**
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
*
*
 */


int main() {
    Position pos;

    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
    //pos.from_fen("rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b - - 1 1");
    pos.do_move(Move::make<NORMAL>(E2, E4));
    pos.do_move(Move::make<NORMAL>(E7, E6));
    //pos.do_move(Move::make<NORMAL>(D7, D5));
    //pos.do_move(Move::make<NORMAL>(E8, D5));
    pos.from_fen("kb1q4/1nb5/8/2Pp1N2/8/8/3R4/K2R4 w - d6 0 1");
    //pos.from_fen("kb1q4/1n6/8/2Pp1N2/8/8/3R4/K2R4 w - d6 0 1");
    std::cout << pos << std::endl;
    std::cout << pos.see(Move::make<EN_PASSANT>(C5, D6)) << std::endl;
    pos.from_fen("k6b/8/8/4r3/3Q4/8/7B/K7 w - - 0 1");
    std::cout << pos.see(Move::make<NORMAL>(D4, E5)) << std::endl;
    std::cout << pos.see(Move::make<NORMAL>(H2, E5)) << std::endl;

    pos.from_fen("rnbqkb1r/1p1p1ppp/B1p2n2/4p2Q/4P3/P1N5/1PPP1PPP/R1B1K1NR b - - 0 1");
    pos.from_fen("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w - - 2 2");
    pos.from_fen("rnbqk2r/pppp1ppp/5n2/4p3/1b2P3/2NB1P2/PPPP2PP/R1BQK1NR b - - 2 2");

    //pos.do_move(Move::make<NORMAL>(E1, E2));

    std::cout << pos << std::endl;

    Eval::NNUE<512> nnue;
    nnue.init(pos);

    std::cout << nnue.evaluate(BLACK);

    g_tt.init(512);
    //tb_init("/home/paul/code/ChePP/scripts/syzyy");

    auto engine = std::make_unique<Engine>(pos);

    const auto start = std::chrono::high_resolution_clock::now();
    volatile int32_t acc = 0;
    for  (int i = 0; i < 1'000'000'000; i++)
    {
        acc += nnue.evaluate(pos.color());
    }
    const auto end = std::chrono::high_resolution_clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "checksum " << acc << " time " <<  delta.count() << std::endl;

    SearchInfo info{30, 10000 };
    std::string fen;
    fen = "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w - - 2 2";
    //fen = "8/1k6/8/2Q1K3/8/8/8/8 w - - 1 2";
    fen = "r1b1k2r/pppp1ppp/2n2n2/8/4q3/P1Q5/1PP1NPPP/R1B1KB1R w - - 0 8";
    fen = "r1b1k2r/pppp1ppp/2n2n2/6B1/4q3/P1Q5/1PP1NPPP/R3KB1R b - - 1 8";
    pos.from_fen(fen);

    for (int i = 0; i < 20; i++)
    {
        g_tt.new_generation();
        SearchThreadHandler handle{1, info, pos};
        handle.start();
        handle.join();
        Move best = handle.get_best_move();
        pos.do_move(best);
        std::cout << pos << std::endl;
    }



    return 0;
}


