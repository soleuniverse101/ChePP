#include "bitboard.h"
#include <sstream>

bitboard_t bb_from_to[NB_SQUARES][NB_SQUARES];
bitboard_t bb_piece_default_attack[NB_PIECES - KNIGHT][NB_SQUARES];
bitboard_t bb_pawn_default_attack[NB_COLORS][NB_SQUARES];

void init_bb_from_to()
{
    for (square_t sq1 = A1; sq1 < NB_SQUARES; ++sq1)
    {
        for (square_t sq2 = A1; sq2 < NB_SQUARES; ++sq2)
        {
            int df = file_of(sq2) - file_of(sq1);
            int dr = rank_of(sq2) - rank_of(sq1);
            if (dr == 0) // same file
            {
                bb_from_to[sq1][sq2] = df < 0 ? sliding_attack_bb<WEST>(sq1, 0, -df)
                                              : sliding_attack_bb<EAST>(sq1, 0, df);
            }
            if (df == 0)
            {
                bb_from_to[sq1][sq2] = dr < 0 ? sliding_attack_bb<SOUTH>(sq1, 0, -dr)
                                              : sliding_attack_bb<NORTH>(sq1, 0, dr);
            }
            if (df == dr)
            {
                bb_from_to[sq1][sq2] = dr < 0
                                           ? (df < 0 ? sliding_attack_bb<SOUTH_WEST>(sq1, 0, -df)
                                                     : sliding_attack_bb<SOUTH_EAST>(sq1, 0, df))
                                           : (df < 0 ? sliding_attack_bb<NORTH_WEST>(sq1, 0, -df)
                                                     : sliding_attack_bb<NORTH_EAST>(sq1, 0, df));
            }
            bb_from_to[sq1][sq2] |= square_to_bb(sq1);
        }
    }
}

void init_bb_piece_default_attacks()
{
    for (square_t sq1 = A1; sq1 < NB_SQUARES; ++sq1)
    {
        for (square_t sq2 = A1; sq2 < NB_SQUARES; ++sq2)
        {
            bb_piece_default_attack[KNIGHT - KNIGHT];
            for (int offset : {})
        }
    }
}

std::string bb_format_string(bitboard_t bb)
{
    std::ostringstream out;
    out << "__________________\n";
    for (rank_t rank = RANK_1; rank <= RANK_8; ++rank)
    {
        out << "|";
        for (file_t file = FILE_A; file <= FILE_H; ++file)
        {
            out << ((bb & square_to_bb((square_t)((NB_RANKS - rank - 1) * 8 + file))) ? "X "
                                                                                      : ". ");
        }
        out << "|\n";
    }
    out << "__________________\n";
    return out.str();
}

int main(void)
{
    init_bb_from_to();
    bitboard_t bb = bb_from_to[A1][G7];
    int        df = file_of(A2) - file_of(B2);
    int        dr = rank_of(A2) - rank_of(B2);
    std::cout << dr << " " << df << std::endl;
    std::cout << bb_format_string(bb_from_to[B2][A2])
              << bb_format_string(sliding_attack_bb<SOUTH_WEST, EAST>(C3, 0, 2)) << std::endl;
    assert(bb_from_to[B2][A1] == sliding_attack_bb<SOUTH_WEST>(C3, 0, 2));
    std::cout << bb_format_string(bb);
    std::cout << bb_format_string(sliding_attack_bb<NORTH, SOUTH, NORTH_WEST>(C2));
}
