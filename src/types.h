#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED
#include <array>
#include <cassert>
#include <stdint.h>

#define ENUM_INCR_OP(T)                                                                            \
    constexpr T& operator++(T& e)                                                                  \
    {                                                                                              \
        return e = T(static_cast<int>(e) + 1);                                                     \
    }                                                                                              \
    constexpr T operator++(T& e, int)                                                              \
    {                                                                                              \
        T old = e;                                                                                 \
        ++e;                                                                                       \
        return old;                                                                                \
    }                                                                                              \
    constexpr T& operator--(T& e)                                                                  \
    {                                                                                              \
        return e = T(static_cast<int>(e) - 1);                                                     \
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
        return T(static_cast<int>(lhs) + rhs);                                                     \
    }                                                                                              \
    constexpr T operator-(T lhs, int rhs)                                                          \
    {                                                                                              \
        return T(static_cast<int>(lhs) - rhs);                                                     \
    }                                                                                              \
    constexpr T operator*(T lhs, int rhs)                                                          \
    {                                                                                              \
        return T(static_cast<int>(lhs) * rhs);                                                     \
    }                                                                                              \
                                                                                                   \
    constexpr T& operator+=(T& lhs, int rhs)                                                       \
    {                                                                                              \
        return lhs = T(static_cast<int>(lhs) + rhs);                                               \
    }                                                                                              \
    constexpr T& operator-=(T& lhs, int rhs)                                                       \
    {                                                                                              \
        return lhs = T(static_cast<int>(lhs) - rhs);                                               \
    }                                                                                              \
    constexpr T& operator*=(T& lhs, int rhs)                                                       \
    {                                                                                              \
        return lhs = T(static_cast<int>(lhs) * rhs);                                               \
    }

using bitboard_t = uint64_t;

// clang-format off
enum square_t : int8_t {
  NO_SQUARE = -1,
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  NB_SQUARES
};
ENUM_INCR_OP(square_t)
ENUM_ARITH_OP(square_t)
constexpr bool is_ok(square_t sq) {
  return (sq >= A1 && sq <= H8);
}

template <typename T>
using all_squares = std::array<T, NB_SQUARES>;
// clang-format on

enum file_t : int8_t
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
};
ENUM_INCR_OP(file_t)
ENUM_ARITH_OP(file_t)
constexpr bool is_ok(const file_t fl)
{
    return (fl >= FILE_A && fl <= FILE_H);
}
constexpr file_t fl_of(const square_t sq)
{
    return static_cast<file_t>(static_cast<int8_t>(sq) & 7);
}

enum rank_t : int8_t
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
};

ENUM_INCR_OP(rank_t)
ENUM_ARITH_OP(rank_t)

constexpr bool is_ok(rank_t rk)
{
    return (rk >= RANK_1 && rk <= RANK_8);
}

constexpr rank_t rk_of(const square_t sq)
{
    return static_cast<rank_t>(static_cast<int8_t>(sq) >> 3);
}

typedef enum piece_type_t : int8_t
{
    NO_PIECE_TYPE = -1,
    PAWN          = 0,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NB_PIECE_TYPES
} piece_type_t;

template <typename T>
using all_piece_types = std::array<T, NB_PIECE_TYPES>;

typedef enum color_t : int8_t
{
    WHITE,
    BLACK,
    NB_COLORS = 3,
    ANY
} color_t;

template <typename T>
using all_colors = std::array<T, NB_COLORS>;

enum piece_t : int8_t
{
    NO_PIECE = -1,
    W_PAWN   = 0,
    W_KNIGHT,
    W_BISHOP,
    W_ROOK,
    W_QUEEN,
    W_KING,
    B_PAWN,
    B_BISHOP,
    B_ROOK,
    B_QUEEN,
    B_KING,
    NB_PIECES
};

template <typename T>
using all_pieces = std::array<T, NB_PIECES>;

enum direction_t : int8_t
{
    NORTH      = 8,
    EAST       = 1,
    SOUTH      = -NORTH,
    WEST       = -EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    NB_DIRECTIONS,
    INVALID = 0
};
template <direction_t D>
constexpr direction_t inverse_dir()
{
    return static_cast<direction_t>(-D);
}

constexpr direction_t direction_from(square_t a, square_t b)
{
    int dr = rk_of(b) - rk_of(a);
    int df = fl_of(b) - fl_of(a);

    if (dr == 0 && df > 0)
        return EAST;
    if (dr == 0 && df < 0)
        return WEST;
    if (df == 0 && dr > 0)
        return NORTH;
    if (df == 0 && dr < 0)
        return SOUTH;
    if (dr == df && dr > 0)
        return SOUTH_EAST;
    if (dr == df && dr < 0)
        return SOUTH_WEST;
    if (dr == -df && dr > 0)
        return NORTH_WEST;
    if (dr == -df && dr < 0)
        return SOUTH_EAST;

    return INVALID;
}

constexpr color_t operator!(const color_t c)
{
    assert(c == WHITE || c == BLACK);
    return color_t(int(c) ^ 1);
}
constexpr color_t operator~(const color_t c)
{
    assert(c == WHITE || c == BLACK);
    return color_t(int(c) ^ 1);
}
template <std::unsigned_integral T>
constexpr int popcount(const T x) noexcept
{
    return std::popcount(x);
}

template <std::unsigned_integral T>
constexpr int get_lsb(const T bb) noexcept
{
    return std::countr_zero(bb);
}

template <std::unsigned_integral T>
constexpr int pop_lsb(T& bb) noexcept
{
    int n = std::countr_zero(bb);
    bb &= ~(static_cast<T>(1) << n);
    return n;
}

#endif
