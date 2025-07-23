#include "bitboard.h"
#include "cstring"
#include <sstream>

using namespace Bitboard;

all_squares<all_squares<bitboard_t>>     bb::from_to{};
all_piece_types<all_squares<bitboard_t>> bb::piece_pseudo_attacks{};
all_colors<all_squares<bitboard_t>>      bb::pawn_pseudo_attacks{};

void init_from_to()
{
    for (square_t sq1 = A1; sq1 < NB_SQUARES; ++sq1)
    {
        for (square_t sq2 = A1; sq2 < NB_SQUARES; ++sq2)
        {
            const int df = fl_of(sq2) - fl_of(sq1);
            const int dr = rk_of(sq2) - rk_of(sq1);
            if (dr == 0) // same file
            {
                from_to.at(sq1).at(sq2) = df < 0 ? ray<WEST>(sq1, 0, -df) : ray<EAST>(sq1, 0, df);
            }
            if (df == 0) // same rank
            {
                from_to.at(sq1).at(sq2) = dr < 0 ? ray<SOUTH>(sq1, 0, -dr) : ray<NORTH>(sq1, 0, dr);
            }
            if (df == dr) // diagonal
            {
                from_to.at(sq1).at(sq2) =
                    dr < 0 ? (df < 0 ? ray<SOUTH_WEST>(sq1, 0, -df) : ray<SOUTH_EAST>(sq1, 0, df))
                           : (df < 0 ? ray<NORTH_WEST>(sq1, 0, -df) : ray<NORTH_EAST>(sq1, 0, df));
            }
            from_to.at(sq1).at(sq2) |= (sq_mask(sq1) | sq_mask(sq2));
        }
    }
}

void init_pseudo_attacks()
{
    for (square_t sq = A1; sq < NB_SQUARES; ++sq)
    {
        const bitboard_t bb                      = bb::sq_mask(sq);
        bb::pawn_pseudo_attacks.at(WHITE).at(sq) = shift<NORTH_WEST>(bb) | shift<NORTH_EAST>(bb);
        bb::pawn_pseudo_attacks.at(BLACK).at(sq) = shift<SOUTH_WEST>(bb) | shift<SOUTH_EAST>(bb);
        bb::piece_pseudo_attacks.at(KNIGHT).at(sq) =
            shift<NORTH, NORTH, EAST>(bb) | shift<NORTH, NORTH, WEST>(bb) |
            shift<SOUTH, SOUTH, EAST>(bb) | shift<SOUTH, SOUTH, WEST>(bb) |
            shift<EAST, EAST, NORTH>(bb) | shift<EAST, EAST, SOUTH>(bb) |
            shift<WEST, WEST, NORTH>(bb) | shift<WEST, WEST, SOUTH>(bb);
        bb::piece_pseudo_attacks.at(BISHOP).at(sq) = ray<BISHOP>(sq);
        bb::piece_pseudo_attacks.at(ROOK).at(sq)   = ray<ROOK>(sq);
        bb::piece_pseudo_attacks.at(QUEEN).at(sq)  = ray<BISHOP>(sq) | ray<ROOK>(sq);
        bb::piece_pseudo_attacks.at(KING).at(sq)   = shift<NORTH>(bb) | shift<SOUTH>(bb) |
                                                   shift<EAST>(bb) | shift<WEST>(bb) |
                                                   shift<NORTH, EAST>(bb) | shift<NORTH, WEST>(bb) |
                                                   shift<SOUTH, EAST>(bb) | shift<SOUTH, WEST>(bb);
    }
}

void Bitboard::init()
{
    init_from_to();
    init_pseudo_attacks();
}

std::string Bitboard::string(bitboard_t bb)
{
    std::ostringstream out;
    out << "__________________\n";
    for (rank_t rank = RANK_1; rank <= RANK_8; ++rank)
    {
        out << "|";
        for (file_t file = FILE_A; file <= FILE_H; ++file)
        {
            out << ((bb & sq_mask((square_t)((NB_RANKS - rank - 1) * 8 + file))) ? "X " : ". ");
        }
        out << "|\n";
    }
    out << "__________________\n";
    return out.str();
}
