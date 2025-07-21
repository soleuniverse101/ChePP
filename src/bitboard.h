#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdint.h>

#define ENUM_INCR_OP(T)                                                                            \
    constexpr T& operator++(T& e)                                                                  \
    {                                                                                              \
        return e = T(int(e) + 1);                                                                  \
    }                                                                                              \
    constexpr T operator++(T& e, int)                                                              \
    {                                                                                              \
        T old = e;                                                                                 \
        ++e;                                                                                       \
        return old;                                                                                \
    }                                                                                              \
    constexpr T& operator--(T& e)                                                                  \
    {                                                                                              \
        return e = T(int(e) - 1);                                                                  \
    }                                                                                              \
    constexpr T operator--(T& e, int)                                                              \
    {                                                                                              \
        T old = e;                                                                                 \
        --e;                                                                                       \
        return old;                                                                                \
    }

#define ENUM_ARITH_OP(T)                                                                           \
    constexpr T operator+(T lhs, int rhs)                                                          \
    {                                                                                              \
        return T(int(lhs) + rhs);                                                                  \
    }                                                                                              \
    constexpr T operator-(T lhs, int rhs)                                                          \
    {                                                                                              \
        return T(int(lhs) - rhs);                                                                  \
    }                                                                                              \
    constexpr T operator*(T lhs, int rhs)                                                          \
    {                                                                                              \
        return T(int(lhs) * rhs);                                                                  \
    }                                                                                              \
                                                                                                   \
    constexpr T& operator+=(T& lhs, int rhs)                                                       \
    {                                                                                              \
        return lhs = T(int(lhs) + rhs);                                                            \
    }                                                                                              \
    constexpr T& operator-=(T& lhs, int rhs)                                                       \
    {                                                                                              \
        return lhs = T(int(lhs) - rhs);                                                            \
    }                                                                                              \
    constexpr T& operator*=(T& lhs, int rhs)                                                       \
    {                                                                                              \
        return lhs = T(int(lhs) * rhs);                                                            \
    }

typedef uint64_t bitboard_t;

// clang-format off
typedef enum square_t : int8_t {
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
ENUM_INCR_OP(square_t)
ENUM_ARITH_OP(square_t)
// clang-format on

typedef enum file_t : int8_t
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
ENUM_INCR_OP(file_t)
ENUM_ARITH_OP(file_t)

typedef enum rank_t : int8_t
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
ENUM_INCR_OP(rank_t)
ENUM_ARITH_OP(rank_t)

typedef enum piece_t : int8_t
{
    NO_PIECE = -1,
    PAWN     = 0,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NB_PIECES
} piece_t;

typedef enum color_t : int8_t
{
    WHITE,
    BLACK,
    NB_COLORS
} color_t;

constexpr bitboard_t bb_empty  = 0ULL;
constexpr bitboard_t bb_full   = ~bb_empty;
constexpr bitboard_t bb_file_a = 0x0101010101010101ULL;
constexpr bitboard_t bb_rank_1 = 0x00000000000000FFULL;

constexpr bitboard_t file_bb(file_t file)
{
    return bb_file_a << int(file);
}

constexpr bitboard_t rank_bb(rank_t rank)
{
    return bb_rank_1 << (int(rank) * 8);
}

constexpr file_t file_of(square_t sq)
{
    return file_t(int(sq) & 7);
}

constexpr rank_t rank_of(square_t sq)
{
    return rank_t(int(sq) >> 3);
}

constexpr bitboard_t square_to_bb(square_t sq)
{
    return 1ULL << int(sq);
}

constexpr bitboard_t file_bb(square_t sq)
{
    return file_bb(file_of(sq));
}

constexpr bitboard_t rank_bb(square_t sq)
{
    return rank_bb(rank_of(sq));
}

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
template <direction_t... dirs>
constexpr int directions_offset()
{
    return (direction_offset<dirs>() + ...);
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
constexpr bitboard_t move_dir_bb_base(bitboard_t bb)
{
    int        off  = direction_offset<dir>();
    bitboard_t mask = direction_mask<dir>();
    return off > 0 ? (bb & mask) << off : (bb & mask) >> -off;
}

template <direction_t First, direction_t... Rest>
constexpr bitboard_t move_dir_bb(bitboard_t bb)
{
    bitboard_t moved = move_dir_bb_base<First>(bb);
    if constexpr (sizeof...(Rest) == 0)
        return moved;
    else
        return move_dir_bb<Rest...>(moved);
}

template <direction_t Dir>
constexpr bitboard_t sliding_attack_bb_base(square_t sq, int blockers = 0, int len = 8)
{
    bitboard_t attacks = 0;
    bitboard_t bb      = move_dir_bb<Dir>(square_to_bb(sq));

    for (int i = 0; i < len && bb; ++i)
    {
        attacks |= bb;
        if (bb & blockers)
            break;
        bb = move_dir_bb<Dir>(bb);
    }

    return attacks;
}

template <direction_t... Dirs>
constexpr bitboard_t sliding_attack_bb(square_t sq, int blockers = 0, int len = 8)
{
    return (sliding_attack_bb_base<Dirs>(sq, blockers, len) | ...);
}

extern bitboard_t bb_from_to[NB_SQUARES][NB_SQUARES];
extern bitboard_t bb_piece_base_attack[NB_PIECES - KNIGHT][NB_SQUARES];
extern bitboard_t bb_pawn_base_attack[NB_COLORS][NB_SQUARES];

template <piece_t pc>
constexpr bitboard_t base_attack_bb(square_t sq, color_t c)
{
    return pc == PAWN ? bb_pawn_base_attack[c][sq] : bb_piece_base_attack[pc - KNIGHT][sq];
}

void        init_bb_from_to();
void        init_bb_base_attacks();
std::string bb_format_string(bitboard_t bb);
