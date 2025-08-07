//
// Created by paul on 7/25/25.
//

#ifndef MOVE_H
#define MOVE_H
#include "castle.h"
#include "types.h"

enum move_type_t : uint16_t
{
    NORMAL     = 0,
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14
};

// a move is encoded as an 16 bit unsigned int
// 0-5 bit : to square (square 0 to 63)
// 6-11 bit : from square (square 0 to 63)
// 12-13 bit : promotion piece type (shifted by KNIGHT which is the lowest promotion to fit) or
// castle type 14-15: promotion (1), en passant (2), castling (3)
class move_t
{
  public:
    move_t() : m_data(0) {}
    constexpr explicit move_t(const std::uint16_t d) : m_data(d) {}

    constexpr move_t(const square_t from, const square_t to) : m_data((index(from) << 6) + index(to)) {}

    // to build a move if you already know the type of move
    template <move_type_t T>
    static constexpr move_t make(const square_t from, const square_t to, const piece_type_t pt = KNIGHT)
    {
        assert(T != CASTLING);
        return move_t{static_cast<uint16_t>(T + (index(pt - KNIGHT) << 12) + (index(from) << 6) + index(to))};
    }

    template <move_type_t T>
    static constexpr move_t make(const square_t from, const square_t to, const castling_type_t c)
    {
        assert(T == CASTLING);
        return move_t(static_cast<uint16_t>(T + (c<< 12) + (index(from) << 6) + index(to)));
    }

    // for sanity check
    [[nodiscard]] constexpr bool is_ok() const { return none().m_data != m_data && null().m_data != m_data; }

    // for these two moves from and to are the same
    static constexpr move_t null() { return move_t(65); }
    static constexpr move_t none() { return move_t(0); }

    constexpr bool operator==(const move_t& m) const { return m_data == m.m_data; }
    constexpr bool operator!=(const move_t& m) const { return m_data != m.m_data; }

    constexpr explicit operator bool() const { return m_data != 0; }

    [[nodiscard]] constexpr std::uint16_t raw() const { return m_data; }

    [[nodiscard]] constexpr square_t from_sq() const
    {
        assert(is_ok());
        return square_t{(m_data >> 6) & 0x3F};
    }

    [[nodiscard]] constexpr square_t to_sq() const
    {
        assert(is_ok());
        return square_t{m_data & 0x3F};
    }

    [[nodiscard]] constexpr move_type_t type_of() const { return static_cast<move_type_t>(m_data & 0b11 << 14); }

    [[nodiscard]] constexpr piece_type_t promotion_type() const
    {
        return piece_type_t{(m_data >> 12 & 0b11) + KNIGHT};
    }

    [[nodiscard]] constexpr castling_type_t castling_type() const
    {
        return static_cast<castling_type_t>(m_data >> 12 & 0b11);
    }

    [[nodiscard]] std::string to_string() const
    {
        std::ostringstream ss;
        ss << from_sq() << to_sq();
        if (type_of() == PROMOTION)
        {
            ss << promotion_type();
        }
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const move_t mv) { return os << mv.to_string(); }

  private:
    std::uint16_t m_data;
};
#endif // MOVE_H
