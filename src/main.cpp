#include "bitboard.h"
#include "position.h"
#include "movegen.h"
#include <chrono>
#include <iostream>
int main()
{
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
    pos.from_fen("r3kr1r/ppppp1pp/3N4/3B3B/4R3/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << pos;
    std::cout << bb::string(pos.color_occupancy(BLACK));
    std::cout << bb::string(pos.checkers(BLACK));
    std::cout << bb::string(pos.m_state->m_check_mask.at(BLACK));
    std::cout << bb::string(pos.m_state->m_blockers.at(BLACK));
    move_list_t l;
    //gen_pawn_moves<BLACK>(pos, &l);
    gen_pc_moves<ROOK, BLACK>(pos, l);

    pos.from_fen("r3k2r/pppppppp/2N5/3B3B/4R3/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << pos;
    gen_castling<BLACK>(pos, l);

    pos.from_fen("8/8/8/3k4/3pPp2/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 1");
    std::cout << pos;
    gen_pawn_moves<BLACK>(pos, &l);

}
