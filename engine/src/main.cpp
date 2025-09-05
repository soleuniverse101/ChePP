
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
    MoveList l;
    if (pos.color() == WHITE)
        gen_legal<WHITE>(pos, l);
    else
        gen_legal<BLACK>(pos, l);

    size_t total = 0;


    for (auto mv : l)
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


    Eval::NNUE<512> nnue;
    nnue.init(pos);

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

    for (int i = 0; i < 5; i++)
    {
        /**
        std::optional<move_t> player_move = std::nullopt;
        move_list_t moves;
        gen_legal<WHITE>(pos, moves);
        while (!player_move)
        {
            std::string uci_move;
            std::cout << "Your move: ";
            std::cin >> uci_move;
            player_move = pos.from_uci(uci_move);
            if (player_move)
            {
                if (std::ranges::find(moves, *player_move) == moves.end())
                {
                    player_move = std::nullopt;
                }
            }
        }

        pos.do_move(*player_move);
        std::cout << pos;

        **/


        Searcher<WHITE> sw(pos, 20, 2000);
        auto mvw = sw.FindBestMove();
        pos.do_move(mvw);
        std::cout << pos;

        Searcher<BLACK> sb(pos, 20, 2000);
        auto mvb = sb.FindBestMove();
        pos.do_move(mvb);
        std::cout << pos;
    }
    return 0;
}


