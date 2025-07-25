#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED
#include <array>
#include <cassert>
#include <cstdint>

using hash_t      = std::uint64_t;
using bitboard_t = std::uint64_t;

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

template <typename T>
using all_files = std::array<T, NB_FILES>;

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

template <typename T>
using all_ranks = std::array<T, NB_RANKS>;

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

constexpr color_t operator!(const color_t c)
{
    assert(c == WHITE || c == BLACK);
    return static_cast<color_t>(static_cast<int>(c) ^ 1);
}
constexpr color_t operator~(const color_t c)
{
    return !c;
}

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
    B_KNIGHT,
    B_BISHOP,
    B_ROOK,
    B_QUEEN,
    B_KING,
    NB_PIECES
};

constexpr piece_type_t piece_type(const piece_t pc)
{
    return static_cast<piece_type_t>(static_cast<int8_t>(pc) % NB_PIECE_TYPES);
}

constexpr color_t color(const color_t pc)
{
    return static_cast<color_t>(static_cast<int8_t>(pc) > NB_PIECE_TYPES);
}

constexpr piece_t piece(const piece_type_t pc, const color_t c)
{
    return static_cast<piece_t>(static_cast<int8_t>(pc)  + NB_PIECE_TYPES * static_cast<int8_t>(c));
}

constexpr piece_t p = piece(ROOK, WHITE);

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
    static_assert(D != INVALID);
    return static_cast<direction_t>(-D);
}

constexpr direction_t direction_from(const square_t a, const square_t b)
{
    const int       dr = rk_of(b) - rk_of(a);
    const int df = fl_of(b) - fl_of(a);

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

enum castling_rights_t : int8_t {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE      = WHITE_OO | BLACK_OO,
    QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};

template <typename T>
using all_castling_rights = std::array<T, CASTLING_RIGHT_NB>;

template <color_t C>
castling_rights_t castling_rights() noexcept
{
    if constexpr (C == WHITE)
    {
        return WHITE_CASTLING;
    }
    else
    {
        return BLACK_CASTLING;
    }
}



#endif
