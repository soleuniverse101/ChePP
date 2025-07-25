//
// Created by paul on 7/25/25.
//

#ifndef CASTLE_H
#define CASTLE_H

#include "types.h"

enum side_t : int8_t { KINGSIDE = 0, QUEENSIDE = 1 };

enum castling_type_t : int8_t
{
    WHITE_OO    = 0,
    WHITE_OOO   = 1,
    BLACK_OO    = 2,
    BLACK_OOO   = 3,
    NB_CASTLING_TYPES = 4,
    NO_CASTLING = -1
};

inline std::array castling_types{WHITE_OO, WHITE_OOO, BLACK_OO, BLACK_OOO};

template <typename T>
using all_castling_types = std::array<T, NB_CASTLING_TYPES>;



class castling_rights_t {
public:
    static constexpr int8_t WHITE_KINGSIDE  = 0b0001;
    static constexpr int8_t WHITE_QUEENSIDE = 0b0010;
    static constexpr int8_t BLACK_KINGSIDE  = 0b0100;
    static constexpr int8_t BLACK_QUEENSIDE = 0b1000;

    static constexpr int8_t ALL = WHITE_KINGSIDE | WHITE_QUEENSIDE |
                                   BLACK_KINGSIDE | BLACK_QUEENSIDE;

    struct rook_move_t {
        const square_t from;
        const square_t to;
    };

    struct king_move_t
    {
        const square_t from;
        const square_t to;
    };

    static constexpr castling_type_t type(const color_t c, const side_t s) {
        return static_cast< castling_type_t>(c * 2 + s);
    }

    // Get castling bitmask for a specific castling type
    static constexpr int8_t mask(const castling_type_t type) {
        switch (type) {
            case WHITE_OO:   return WHITE_KINGSIDE;
            case WHITE_OOO:  return WHITE_QUEENSIDE;
            case BLACK_OO:   return BLACK_KINGSIDE;
            case BLACK_OOO:  return BLACK_QUEENSIDE;
            default:         return 0;
        }
    }

    [[nodiscard]] bool has_right(const castling_type_t type) const { return m_rights & mask(type); }

    void remove_right(const castling_type_t type) {
        m_rights &= ~mask(type);
    }

    // Remove all rights for a color
     void remove_color(const color_t c) {
        m_rights = m_rights & ~(mask(type(c, KINGSIDE)) | mask(type(c, QUEENSIDE)));
    }

    // Remove all rights
     void clear_all() {
        m_rights = 0;
    }

    static constexpr rook_move_t rook_move(const castling_type_t type) {
        switch (type) {
            case WHITE_OO:   return {H1, F1};
            case WHITE_OOO:  return {A1, D1};
            case BLACK_OO:   return {H8, F8};
            case BLACK_OOO:  return {A8, D8};
            default:         return {NO_SQUARE, NO_SQUARE};
        }
    }

    static constexpr king_move_t king_move(const castling_type_t type) {
        switch (type) {
            case WHITE_OO:   return {E1, G1};
            case WHITE_OOO:  return {E1, C1};
            case BLACK_OO:   return {E8, G8};
            case BLACK_OOO:  return {E8, C8};
            default:         return {NO_SQUARE, NO_SQUARE};
        }
    }

    static constexpr castling_type_t from_rook_move(const color_t c, const square_t from, const square_t to) {
        if (c == WHITE) {
            if (from == H1 && to == F1) return WHITE_OO;
            if (from == A1 && to == D1) return WHITE_OOO;
        } else {
            if (from == H8 && to == F8) return BLACK_OO;
            if (from == A8 && to == D8) return BLACK_OOO;
        }
        return NO_CASTLING;
    }

    // Returns the bitmask of castling rights lost by moving from a square
    static constexpr int8_t lost_by_moving_from(const square_t square) {
        switch (square) {
            case E1: return WHITE_KINGSIDE | WHITE_QUEENSIDE;
            case H1: return WHITE_KINGSIDE;
            case A1: return WHITE_QUEENSIDE;
            case E8: return BLACK_KINGSIDE | BLACK_QUEENSIDE;
            case H8: return BLACK_KINGSIDE;
            case A8: return BLACK_QUEENSIDE;
            default: return 0;
        }
    }

    [[nodiscard]] int8_t mask() const { return m_rights; }

private:
    int8_t m_rights = ALL;
};

#endif //CASTLE_H
