#include "ChePP/bitboard.h"
#include "ChePP/movegen.h"
#include "ChePP/position.h"
#include "ChePP/search.h"
#include "ChePP/tt.h"

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
    g_tt.init(2048);


    position_t pos;

    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //pos.from_fen("k7/8/B2NK3/8/8/8/8/8 w - - 0 1");
    //pos.from_fen("r1bq1rk1/pp3ppp/2n1pn2/2bp4/2B1P3/2N2N2/PPP2PPP/R1BQ1RK1 w - - 0 10");
    //pos.from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    std::cout << pos;



    for (int i = 0; i < 30; i++)
    {
        const auto mv = find_best_move<WHITE>(pos, 8);
        g_tt.new_generation();
        pos.do_move(mv);
        std::cout << pos;
        const auto mv2 = find_best_move<BLACK>(pos, 8);
        g_tt.new_generation();
        pos.do_move(mv2);
        std::cout << pos;
    }

}
