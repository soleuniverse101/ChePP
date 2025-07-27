#include "bitboard.h"
#include "position.h"
#include "movegen.h"
#include <chrono>
#include <iostream>


void perft(position_t& pos, const int ply, size_t& out)
{
    move_list_t l;
    if (pos.color() == WHITE)
    {
        gen_legal<WHITE>(pos, l);

    } else
    {
        gen_legal<BLACK>(pos, l);

    }
    if (l.size() == 0)
    {
        //std::cout << pos;
    }
    if (ply == 0)
    {
        out += l.size();
        return;
    }
    for (size_t i = 0; i < l.size(); i++)
    {
        const auto mv = l[i];
        //std::cout << pos;
        //std::cout << piece_to_char(pos.piece_at(mv.from_sq())) << " " << square_to_string(mv.from_sq()) << " " << square_to_string(mv.to_sq()) << " " << (static_cast<int>(mv.type_of()) >> 14) <<  std::endl;
        //std::cout << " " << square_to_string(mv.from_sq()) << " " << square_to_string(mv.to_sq()) << " " << (static_cast<int>(mv.type_of()) >> 14) <<  std::endl;

        pos.do_move(mv);
        //std::cout << pos;

            perft(pos, ply - 1, out);

        pos.undo_move(mv);
    }

}

int main()
{
    zobrist_t::init(0xFADA);
    bb::init();

    position_t pos;

    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //pos.from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    std::cout << pos;
    std::cout << " START PERFT " << std::endl;
    size_t     out   = 0;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    const int depth = 6;
    for (int i = 0; i < 1; i++)
    {    perft(pos, depth - 1, out);

    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "perft " << depth << ": " << diff.count() << "s\n";
    std::cout << "perft " << out << std::endl;

    std::cout << "HELLO"<< std::endl;

}
