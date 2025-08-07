//
// Created by paul on 7/25/25.
//

#ifndef CASTLE_H
#define CASTLE_H

#include "types.h"

#include <sstream>
#include <string>

#include "src/tbprobe.h"

enum side_t : int8_t
{
    KINGSIDE  = 0,
    QUEENSIDE = 1
};

enum castling_type_t : uint8_t
{
    WHITE_OO          = 0,
    WHITE_OOO         = 1,
    BLACK_OO          = 2,
    BLACK_OOO         = 3,
    NB_CASTLING_TYPES = 4,
    NO_CASTLING       = 8
};

inline constexpr std::array<const castling_type_t, NB_CASTLING_TYPES> castling_types{WHITE_OO, WHITE_OOO, BLACK_OO,
                                                                                     BLACK_OOO};
inline constexpr std::array<const char, NB_CASTLING_TYPES>            castling_strings{'K', 'Q', 'k', 'q'};

template <typename T>
using all_castling_types = std::array<T, NB_CASTLING_TYPES>;

class castling_rights_t
{
  public:
    static constexpr uint8_t WHITE_KINGSIDE  = 0b0001;
    static constexpr uint8_t WHITE_QUEENSIDE = 0b0010;
    static constexpr uint8_t BLACK_KINGSIDE  = 0b0100;
    static constexpr uint8_t BLACK_QUEENSIDE = 0b1000;

    //compatibility with tb
    static_assert(WHITE_KINGSIDE == TB_CASTLING_K);
    static_assert(WHITE_QUEENSIDE == TB_CASTLING_Q);
    static_assert(BLACK_KINGSIDE == TB_CASTLING_k);
    static_assert(BLACK_QUEENSIDE == TB_CASTLING_q);

    static constexpr uint8_t ALL = WHITE_KINGSIDE | WHITE_QUEENSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE;

    struct rook_move_t
    {
        const square_t from;
        const square_t to;
    };

    struct king_move_t
    {
        const square_t from;
        const square_t to;
    };

    explicit castling_rights_t(const uint8_t mask)
    {
        assert(mask <= ALL);
        m_rights = mask;
    }

    static constexpr castling_type_t type(const color_t c, const side_t s)
    {
        return static_cast<castling_type_t>(index(c) * 2 + s);
    }

    // Get castling bitmask for a specific castling type
    static constexpr uint8_t mask(const castling_type_t type)
    {
        switch (type)
        {
            case WHITE_OO:
                return WHITE_KINGSIDE;
            case WHITE_OOO:
                return WHITE_QUEENSIDE;
            case BLACK_OO:
                return BLACK_KINGSIDE;
            case BLACK_OOO:
                return BLACK_QUEENSIDE;
            default:
                return 0;
        }
    }

    [[nodiscard]] bool has_right(const castling_type_t type) const { return (m_rights & mask(type)) != 0; }

    void remove_right(const castling_type_t type) { m_rights &= ~mask(type); }

    void remove_right(const uint8_t lose_mask) { m_rights &= ~lose_mask; }

    // Remove all rights for a color
    void remove_color(const color_t c) { m_rights &= ~(mask(type(c, KINGSIDE)) | mask(type(c, QUEENSIDE))); }

    // Remove all rights
    void clear_all() { m_rights = 0; }

    void add(const castling_type_t type)
    {
        assert(type < NB_CASTLING_TYPES);
        m_rights |= mask(type);
    }

    static constexpr rook_move_t rook_move(const castling_type_t type)
    {
        switch (type)
        {
            case WHITE_OO:
                return {H1, F1};
            case WHITE_OOO:
                return {A1, D1};
            case BLACK_OO:
                return {H8, F8};
            case BLACK_OOO:
                return {A8, D8};
            default:
                return {NO_SQUARE, NO_SQUARE};
        }
    }

    static constexpr king_move_t king_move(const castling_type_t type)
    {
        switch (type)
        {
            case WHITE_OO:
                return {E1, G1};
            case WHITE_OOO:
                return {E1, C1};
            case BLACK_OO:
                return {E8, G8};
            case BLACK_OOO:
                return {E8, C8};
            default:
                return {NO_SQUARE, NO_SQUARE};
        }
    }

    static constexpr castling_type_t from_rook_move(const color_t c, const square_t from, const square_t to)
    {
        if (c == WHITE)
        {
            if (from == H1 && to == F1)
                return WHITE_OO;
            if (from == A1 && to == D1)
                return WHITE_OOO;
        }
        else
        {
            if (from == H8 && to == F8)
                return BLACK_OO;
            if (from == A8 && to == D8)
                return BLACK_OOO;
        }
        return NO_CASTLING;
    }

    // Returns the bitmask of castling rights lost by moving from a square
    static constexpr uint8_t lost_by_moving_from(const square_t square)
    {
        switch (index(square))
        {
            case index(E1):
                return WHITE_KINGSIDE | WHITE_QUEENSIDE;
            case index(H1):
                return WHITE_KINGSIDE;
            case index(A1):
                return WHITE_QUEENSIDE;
            case index(E8):
                return BLACK_KINGSIDE | BLACK_QUEENSIDE;
            case index(H8):
                return BLACK_KINGSIDE;
            case index(A8):
                return BLACK_QUEENSIDE;
            default:
                return 0;
        }
    }

    [[nodiscard]] std::string to_string() const
    {
        std::ostringstream out;
        for (const auto t : castling_types)
        {
            if (mask(t) & m_rights)
            {
                out << castling_strings.at(t);
            }
        }
        return out.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const castling_rights_t& crs) { return os << crs.to_string(); }

    static castling_rights_t from_string(const std::string_view str)
    {
        assert(str.size() <= 4);
        uint8_t rights = 0;

        for (const char c : str)
        {
            switch (c)
            {
                case 'K':
                    rights |= WHITE_KINGSIDE;
                    break;
                case 'Q':
                    rights |= WHITE_QUEENSIDE;
                    break;
                case 'k':
                    rights |= BLACK_KINGSIDE;
                    break;
                case 'q':
                    rights |= BLACK_QUEENSIDE;
                    break;
                case '-':
                    assert(str.size() == 1);
                    return castling_rights_t{0};
                default:
                    assert(false && "Invalid castling character in string");
            }
        }
        return castling_rights_t{rights};
    }

    [[nodiscard]] uint8_t mask() const { return m_rights; }

  private:
    uint8_t m_rights;
};

#endif // CASTLE_H
