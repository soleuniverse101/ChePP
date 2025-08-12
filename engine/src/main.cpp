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

inline int king_square_index(square_t relative_king_square) {

    // clang-format off
    constexpr enum_array<square_t, int> indices{
        0,  1,  2,  3,  3,  2,  1,  0,
        4,  5,  6,  7,  7,  6,  5,  4,
        8,  9,  10, 11, 11, 10, 9,  8,
        8,  9,  10, 11, 11, 10, 9,  8,
        12, 12, 13, 13, 13, 13, 12, 12,
        12, 12, 13, 13, 13, 13, 12, 12,
        14, 14, 15, 15, 15, 15, 14, 14,
        14, 14, 15, 15, 15, 15, 14, 14,
    };
    // clang-format on

    return indices[relative_king_square];
}

inline int index(const piece_t piece, const square_t piece_square, const square_t king_square, const color_t view)

{
    constexpr int          PIECE_TYPE_FACTOR  = 64;
    constexpr int          PIECE_COLOR_FACTOR = PIECE_TYPE_FACTOR * 6;
    constexpr int          KING_SQUARE_FACTOR = PIECE_COLOR_FACTOR * 2;

    square_t          relative_king_square = NO_SQUARE;
    square_t          relative_piece_square = NO_SQUARE;

    if (view == WHITE) {
        relative_king_square  = king_square;
        relative_piece_square = piece_square;
    } else {
        relative_king_square  = king_square.flipped_horizontally();
        relative_piece_square = piece_square.flipped_horizontally();
    }

    const int king_square_idx = king_square_index(relative_king_square);
    if (index(king_square.file()) > 3) {
        relative_piece_square = relative_piece_square.flipped_vertically();
    }

    const int index = value_of(relative_piece_square) + value_of(piece.type()) * PIECE_TYPE_FACTOR
                      + (piece.color() == view) * PIECE_COLOR_FACTOR
                      + king_square_idx * KING_SQUARE_FACTOR;

    return index;
}

int main() {
    position_t pos;
    bool valid = pos.from_fen("rnbqkbnr/pppppppp/8/8/8/P7/PPPPPPPP/RNBQKBNR w - - 0 1");
    if (!valid)
    {
        throw std::invalid_argument("invalid position");
    }

    std::cout << pos;

    //FileSource weight_source{"engine/src/network.net"};
    MmapSource weight_source{network_net};
    Deserializer deserializer{weight_source};
    DotNetParser parser{deserializer};

    using AccumulatorT = Accumulator<int16_t, 512, 16 * 12 * 64>;
    using ReluT = ReLU<int16_t, 1024>;
    using DenseT = Dense<int16_t, int32_t, 1024, 1>;
    using QuantT = Quantizer<int32_t, 1, 32 * 128>;
    //using SigmoidT = Sigmoid<float, 1, 2.5f/400>;

    using Net = NNUE<AccumulatorT, ReluT, DenseT, QuantT>;

    Net net{};
    parser.load_network(net);

    std::pair<AccumulatorInput, AccumulatorInput> acc_in {};
    auto& [acc_in_w, acc_in_b] = acc_in;
    acc_in_w.clear();
    acc_in_b.clear();

    for (square_t sq = A1; sq <= H8; ++sq)
    {
        if (pos.piece_at(sq) == NO_PIECE) continue;
        int idx_w = index(pos.piece_at(sq), sq, pos.ksq(WHITE), WHITE);
        int idx_b = index(pos.piece_at(sq), sq, pos.ksq(BLACK), BLACK);
        acc_in_w.add<true>(idx_w);
        acc_in_b.add<true>(idx_b);
    }


    DenseBuffer<int32_t, 1> output;

    net.forward(acc_in, output);
    std::cout << output[0] << '\n';

    return 0;
}
