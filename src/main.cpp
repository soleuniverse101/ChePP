#include "bitboard.h"
#include <chrono>
#include <iostream>
int main(void)
{
    bb::init();
    std::cout << Bitboard::string(bb::attacks<QUEEN>(D5, bb::sq_mask(E6)));
    std::cout << Bitboard::string(bb::attacks<ROOK>(H8, bb::attacks<QUEEN>(D5, 0)));
    std::cout << Bitboard::string(bb::attacks<KNIGHT>(D5, bb::attacks<QUEEN>(D5, 0)));
    std::cout << Bitboard::string(bb::attacks<PAWN>(H4, bb::attacks<QUEEN>(D5, 0)));
    const int           iterations = 1'000'000;
    volatile bitboard_t total      = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        for (int sq = 0; sq < 64; ++sq)
        {
            total ^= bb::attacks<QUEEN>((square_t)sq, i);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Movegen benchmark (QUEEN, " << iterations << "x64): " << elapsed.count() << "s\n";
    std::cout << "Total (to avoid opt): " << total << "\n";
}
