#include "ChePP/bitboard.h"
#include "ChePP/movegen.h"
#include "ChePP/position.h"
#include <chrono>
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

        std::cout << piece_to_char(pos.piece_at(mv.from_sq())) << " " << square_to_string(mv.from_sq()) << " " << square_to_string(mv.to_sq()) << ": " << nodes << '\n';
        total += nodes;
    }

    std::cout << "Total: " << total << '\n';
}


int main()
{
    zobrist_t::init(0xFADA);
    bb::init();

    position_t pos;

    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
     pos.from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    //pos.from_fen("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
    //pos.from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    //pos.from_fen("8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - 1 67");
    //pos.from_fen("8/7p/p5pb/4k3/P1pPn3/8/P5PP/1rB2RK1 b - d3 0 28");
    //pos.from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ");
    //pos.from_fen("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
    //pos.do_move(move_t::make<NORMAL>(D7, C6));
//pos.do_move(move_t::make<PROMOTION>(B7, A8, ROOK));
    pos.from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ");
    pos.from_fen("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1 ");
    pos.from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    pos.from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    pos.from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    pos.from_fen("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1");
    pos.from_fen("8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - 1 67");
    pos.from_fen("8/7p/p5pb/4k3/P1pPn3/8/P5PP/1rB2RK1 b - d3 0 28");
    pos.from_fen("rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    pos.from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    pos.from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");





    std::cout << pos;
    std::cout << " START PERFT " << std::endl;
    size_t     out   = 0;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    const int depth = 6;
     perft(pos, depth, out);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "perft " << depth << ": " << diff.count() << "s\n";
    std::cout << "perft " << out << std::endl;

    std::cout << "HELLO"<< std::endl;

    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    for (int d = 1; d<=1; d++)
    {
        constexpr uint64_t expected[] = {0ULL, 20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL};
        out = 0;
        perft(pos, d, out);
        perft(pos, d, out);
        std::cout << out << std::endl;
    }

}
