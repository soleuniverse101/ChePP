#include "bitboard.h"
#include "position.h"
#include "movegen.h"
#include <chrono>
#include <iostream>
int main()
{
    std::cout << "HELLO" << std::endl;
    zobrist_t::init(0xFADA);
    bb::init();
    std::cout << bb::string(bb::attacks<QUEEN>(D5, bb::sq_mask(E6)));
    std::cout << bb::string(bb::attacks<ROOK>(H8, bb::attacks<QUEEN>(D5, 0)));
    std::cout << bb::string(bb::attacks<KNIGHT>(D5, bb::attacks<QUEEN>(D5, 0)));
    std::cout << bb::string(bb::attacks<PAWN>(H4, bb::attacks<QUEEN>(D5, 0)));
    std::cout << bb::string(bb::from_to_incl(B2, G7));
    std::cout << bb::string(bb::g_lines.at(A1).at(C3));
    constexpr int           iterations = 1'000'000;
    volatile bitboard_t     total      = 0;

    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        for (int sq = 0; sq < 64; ++sq)
        {
            total ^= bb::attacks<QUEEN>(static_cast<square_t>(sq), i);
        }
    }
    const auto end = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<double> elapsed = end - start;
    std::cout << "Movegen benchmark (QUEEN, " << iterations << "x64): " << elapsed.count() << "s\n";
    std::cout << "Total (to avoid opt): " << total << "\n";

    position_t pos;
    pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << pos;

    move_list_t l1;
    gen_legal<WHITE>(pos, l1);
    for (size_t i = 0; i < l1.size(); i++)
    {
        const auto mv = l1[i];
        std::cout << piece_to_char(pos.piece_at(mv.from_sq())) << " " << square_to_string(mv.from_sq()) << " " << square_to_string(mv.to_sq()) << std::endl;
    }
    //gen_pawn_moves<BLACK>(pos, &l);
    std::cout << "the nb of legal moves is " << l1.size() << std::endl;
    l1.clear();
    pos.from_fen("r3k2r/pppppppp/2N5/3B3B/4R3/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << pos;
    gen_castling<BLACK>(pos, l1);
    move_list_t l;
    l.clear();
    pos.from_fen("8/8/8/3k4/3pPp2/2QR4/PPPPPPPP/RNBQKBNR w KQ e3 0 1");
    std::cout << pos;
    std::cout << pos.crs().to_string() << std::endl;
    gen_legal<BLACK>(pos, l);
    for (size_t i = 0; i < l.size(); i++)
    {
        const auto mv = l[i];
        std::cout << piece_to_char(pos.piece_at(mv.from_sq())) << " " << square_to_string(mv.from_sq()) << " " << square_to_string(mv.to_sq()) << " " << (static_cast<int>(mv.type_of()) >> 14) <<  std::endl;
        pos.do_move(mv);
        std::cout << pos;
    }
    std::cout << "HELLO"<< std::endl;

}
