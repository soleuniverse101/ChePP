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
**/

int main() {
    bb::init();
    zobrist_t::init(0xFADA);
    g_tt.init(512);
    tb_init("/home/paul/code/ChePP/scripts/syzyy");


    position_t pos;
    if (!pos.from_fen("1kr5/3n4/q3p2p/p2n2p1/PppB1P2/5BP1/1P2Q2P/3R2K1 w - - 0 1"))
    {
        throw std::invalid_argument("invalid position");
    }
    //pos.from_fen("1kr5/3n4/q3p2p/p2n2p1/PppB1P2/5BP1/1P2Q2P/3R2KQ w - - 0 1");
    pos.from_fen("1q2bn2/6pk/2p1pr1p/2Q2p1P/1PP5/5N2/5PP1/4RBK1 w - - 0 1");
    //pos.from_fen("8/Q5pk/5rnp/5B1q/1PPp4/4R1P1/5P2/3b2K1 w - - 0 9");
    //pos.from_fen("1k2r3/1p1bP3/2p2p1Q/Ppb5/4Rp1P/2q2N1P/5PB1/6K1 b - - 0 1");
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos.from_fen("r2q1rk1/4bppp/1pn1p3/p4b2/1nNP4/4BN2/PP2BPPP/R2Q1RK1 w - - 2 13");

    std::cout << pos;

    //FileSource weight_source{"engine/src/network.net"};
    MmapSource weight_source{network_net};
    const Deserializer deserializer{weight_source};
    const DotNetParser parser{deserializer};

    Koi::Net net{
        Koi::ReluT{},
        Koi::DenseT{},
        Koi::QuantT{},
    };
    parser.load_network(net);

    for (int i = 0; i < 50; i++)
    {
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


        Searcher<BLACK> sw(pos, net, 20, 2000);
        auto mv = sw.FindBestMove();
        pos.do_move(mv);
        std::cout << pos;






    }

    return 0;
}
