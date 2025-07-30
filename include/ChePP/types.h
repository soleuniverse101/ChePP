#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <sys/stat.h>

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

constexpr std::array squares = {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};
// clang-format on

constexpr bool is_ok(const square_t sq) { return (sq >= A1 && sq <= H8) || (sq == NO_SQUARE); }

inline int mirror_sq(const int sq) {
    return (7 - (sq / 8)) * 8 + (sq % 8);
}

template <typename T>
using all_squares = std::array<T, NB_SQUARES>;



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

constexpr bool is_ok(const file_t f) { return f >= FILE_A && f <= FILE_H;}

constexpr std::array files = { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

template <typename T>
using all_files = std::array<T, NB_FILES>;

constexpr file_t fl_of(const square_t sq)
{
    assert(is_ok(sq));
    return static_cast<file_t>(static_cast<int8_t>(sq) & 7);
}

constexpr char file_to_char(const file_t f) {
    assert(is_ok(f));
    return static_cast<char>('a' + f);
}

inline file_t char_to_file(const char c) {
    assert(c >= 'a' && c <= 'h');
    return static_cast<file_t>(c - 'a');
}

inline std::ostream& operator<<(std::ostream& os, const file_t f) {
    return os << file_to_char(f);
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

constexpr bool is_ok(const rank_t r) { return r >= RANK_1 && r <= RANK_8;}

constexpr std::array ranks = { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

template <typename T>
using all_ranks = std::array<T, NB_RANKS>;

constexpr rank_t rk_of(const square_t sq)
{
    return static_cast<rank_t>(static_cast<int8_t>(sq) >> 3);
}

constexpr char rank_to_char(const rank_t r) {
    assert(is_ok(r));
    return static_cast<char>('1' + r);
}

constexpr rank_t char_to_rank(const char c) {
    assert(c >= '1' && c <= '8');
    return static_cast<rank_t>(c - '1');
}

inline std::ostream& operator<<(std::ostream& os, const rank_t r) {
    return os << rank_to_char(r);
}



constexpr square_t square(const file_t f, const rank_t r)
{
    assert(is_ok(f) && is_ok(r));
    return static_cast<square_t>(f + r * 8);
}

constexpr std::string_view square_to_string(const square_t sq) {
    assert(is_ok(sq));
    if (sq == NO_SQUARE)
    {
        return "-";
    }
    static char buf[3];
    buf[0] = file_to_char(fl_of(sq));
    buf[1] = rank_to_char(rk_of(sq));
    buf[2] = '\0';
    return {buf, 2};
}

constexpr square_t string_to_square(const std::string_view s) {
    if (s.size() == 1)
    {
        assert(s[0] == '-');
        return NO_SQUARE;
    }
    if (s.size() == 2)
    {
        return square(char_to_file(s[0]), char_to_rank(s[1]));
    }
    assert(false && "invalid square string");
}

inline std::ostream& operator<<(std::ostream& os, const square_t sq) {
    return os << square_to_string(sq);
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



inline int piece_value(const piece_type_t p)
{
    switch (p)
    {
        case PAWN:
            return 100;
        case KNIGHT:
            return 300;
        case BISHOP:
            return 325;
        case ROOK:
            return 500;
        case QUEEN:
            return 900;
        case KING:
            return 10000;
        default:
            return 0;
    }
}


template <typename T>
using all_piece_types = std::array<T, NB_PIECE_TYPES>;

typedef enum color_t : int8_t
{
    WHITE,
    BLACK,
    NB_COLORS,
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

constexpr char color_to_char(const color_t c)
{
    assert(c == WHITE || c == BLACK);
    return c == WHITE ? 'w' : 'b';
}

constexpr color_t char_to_color(const char c)
{
    assert(c == 'w' || c == 'b');
    return static_cast<color_t>(c == 'w' ? WHITE : BLACK);
}

inline std::ostream& operator<<(std::ostream& os, const color_t c) {
    return os << color_to_char(c);
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

constexpr piece_type_t piece_piece_type(const piece_t pc)
{
    return static_cast<piece_type_t>(static_cast<int8_t>(pc) % NB_PIECE_TYPES);
}

constexpr color_t piece_color(const piece_t pc)
{
    return static_cast<color_t>(static_cast<int8_t>(pc) >= NB_PIECE_TYPES);
}

constexpr piece_t piece(const piece_type_t pc, const color_t c)
{
    return static_cast<piece_t>(static_cast<int8_t>(pc)  + NB_PIECE_TYPES * static_cast<int8_t>(c));
}

inline piece_t char_to_piece(const char c)
{
    switch (c)
    {
        case 'P':
            return W_PAWN;
        case 'p':
            return B_PAWN;
        case 'N':
            return W_KNIGHT;
        case 'n':
            return B_KNIGHT;
        case 'B':
            return W_BISHOP;
        case 'b':
            return B_BISHOP;
        case 'R':
            return W_ROOK;
        case 'r':
            return B_ROOK;
        case 'Q':
            return W_QUEEN;
        case 'q':
            return B_QUEEN;
        case 'K':
            return W_KING;
        case 'k':
            return B_KING;
        default:
            assert(0 && "Invalid piece character");
            return NO_PIECE;
    }
}

inline piece_type_t char_to_piece_type(const char c)
{
    return piece_piece_type(char_to_piece(c));
}

inline int piece_value(const piece_t p)
{
    return piece_piece_type(p);
}

template <typename T>
using all_pieces = std::array<T, NB_PIECES>;

inline char piece_to_char(const color_t color, const piece_type_t pt)
{
    static all_colors<all_piece_types<char>> pieces = {{'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'}};
    return pieces.at(color).at(pt);
}

inline char piece_to_char(const piece_t pc)
{
    static all_pieces<char> pieces = {{'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'}};
    return pieces.at(pc);
}

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
constexpr int get_msb(const T bb) noexcept
{
    return std::countl_zero(bb);
}


template <std::unsigned_integral T>
constexpr int pop_lsb(T& bb) noexcept
{
    int n = std::countr_zero(bb);
    bb &= ~(static_cast<T>(1) << n);
    return n;
}

constexpr int MATE_SCORE = 100000;
constexpr int INFINITE = 1000000;


#endif
