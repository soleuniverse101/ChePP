#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdint.h>

typedef uint64_t bitboard_t;

// clang-format off
typedef enum square_t : uint8_t {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  NB_SQUARES
} square_t;
// clang-format on

typedef enum file_t : uint8_t
{
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H,
    NB_FILES
} file_t;

typedef enum rank_t : uint8_t
{
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    NB_RANKS
} rank_t;

constexpr bitboard_t bb_empty  = 0ULL;
constexpr bitboard_t bb_full   = ~bb_empty;
constexpr bitboard_t bb_file_a = 0x0101010101010101ULL;
constexpr bitboard_t bb_rank_1 = 0x00000000000000FFULL;

constexpr bitboard_t file_bb(file_t file)
{
    return bb_file_a << file;
}

constexpr bitboard_t rank_bb(rank_t rank)
{
    return bb_rank_1 << (rank * 8);
}

constexpr file_t file_of(square_t sq)
{
    return static_cast<file_t>(sq & 7);
}

constexpr rank_t rank_of(square_t sq)
{
    return static_cast<rank_t>(sq >> 3);
}

constexpr bitboard_t square_to_bb(square_t sq)
{
    return 1ULL << sq;
}

constexpr bitboard_t file_bb(square_t sq)
{
    return file_bb(file_of(sq));
}

constexpr bitboard_t rank_bb(square_t sq)
{
    return rank_bb(rank_of(sq));
}
#include <cstdint>

typedef uint64_t bitboard_t;

typedef enum direction_t : uint8_t
{
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST,
    NB_DIRECTIONS
} direction_t;

template <direction_t dir>
constexpr int direction_offset()
{
    switch (dir)
    {
        case NORTH:
            return 8;
        case NORTH_EAST:
            return 9;
        case EAST:
            return 1;
        case SOUTH_EAST:
            return -7;
        case SOUTH:
            return -8;
        case SOUTH_WEST:
            return -9;
        case WEST:
            return -1;
        case NORTH_WEST:
            return 7;
        default:
            return 0;
    }
}

template <direction_t dir>
constexpr bitboard_t direction_mask()
{
    switch (dir)
    {
        case EAST:
        case NORTH_EAST:
        case SOUTH_EAST:
            return ~file_bb(FILE_H);
        case WEST:
        case NORTH_WEST:
        case SOUTH_WEST:
            return ~bb_file_a;
        default:
            return bb_full;
    }
}

template <direction_t dir>
constexpr bitboard_t move_dir_bb(bitboard_t bb)
{
    int        off  = direction_offset<dir>();
    bitboard_t mask = direction_mask<dir>();
    return off > 0 ? (bb & mask) << off : (bb & mask) >> -off;
}

template <direction_t dir>
constexpr bitboard_t sliding_attack_bb(square_t sq, int len)
{
    bitboard_t bb = square_to_bb(sq);
    for (int i = 0; i < len; i++)
    {
        bb |= move_dir_bb<dir>(bb);
    }
    return bb;
}

constexpr bitboard_t b1 = sliding_attack_bb<NORTH>(A1, 7);
static_assert(b1 == file_bb(FILE_A));

constexpr bitboard_t b = file_bb(B1);

static bitboard_t bb_from_to[NB_SQUARES][NB_SQUARES];

void init_bb_from_to()
{
    for (uint8_t sq1 = A1; sq1 < NB_SQUARES; ++sq1)
    {
        for (uint8_t sq2 = A1; sq2 < NB_SQUARES; ++sq2)
        {
            int df = file_of((square_t)sq2) - file_of((square_t)sq1);
            int dr = rank_of((square_t)sq2) - rank_of((square_t)sq1);
            if (dr == 0 && df == 0) // same square
            {
                assert(sq1 == sq2);
                bb_from_to[sq1][sq2] = square_to_bb((square_t)sq1);
            }
            if (dr == 0) // same file
            {
                bb_from_to[sq1][sq2] = df < 0 ? sliding_attack_bb<WEST>((square_t)sq1, -df)
                                              : sliding_attack_bb<EAST>((square_t)sq1, df);
            }
            if (df == 0)
            {
                bb_from_to[sq1][sq2] = dr < 0 ? sliding_attack_bb<SOUTH>((square_t)sq1, -dr)
                                              : sliding_attack_bb<NORTH>((square_t)sq1, dr);
            }
            if (df == dr)
            {
                bb_from_to[sq1][sq2] =
                    dr < 0 ? (df < 0 ? sliding_attack_bb<SOUTH_WEST>((square_t)sq1, -df)
                                     : sliding_attack_bb<SOUTH_EAST>((square_t)sq1, df))
                           : (df < 0 ? sliding_attack_bb<NORTH_WEST>((square_t)sq1, -df)
                                     : sliding_attack_bb<NORTH_EAST>((square_t)sq1, df));
            }
        }
    }
}
int main(void)
{
    init_bb_from_to();
    bitboard_t bb = bb_from_to[A1][A7];
    assert(bb_from_to[C3][A1] == sliding_attack_bb<SOUTH_WEST>(C3, 2));
    std::cout << bb;
}
