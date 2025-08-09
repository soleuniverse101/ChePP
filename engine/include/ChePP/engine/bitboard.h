#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED
#include "types.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <string>

class bitboard_t
{
  public:
    using bitboard_type = uint64_t;

    static void        init();

    constexpr bitboard_t() noexcept = default;
    explicit constexpr bitboard_t(const bitboard_type value) noexcept : m_data(value) {}
    constexpr bitboard_t(const bitboard_t& other) noexcept = default;
    constexpr bitboard_t(bitboard_t&& other) noexcept = default;
    constexpr bitboard_t& operator=(const bitboard_t& other) noexcept = default;
    constexpr bitboard_t& operator=(bitboard_t&& other) noexcept = default;
    constexpr bitboard_t& operator=(const bitboard_type value)
    {
        m_data = value;
        return *this;
    }

    explicit constexpr                    operator bitboard_type() const { return m_data; }
    [[nodiscard]] constexpr bitboard_type value() const { return m_data; }


    [[nodiscard]] std::string to_string() const;

    constexpr bitboard_t operator~() const { return bitboard_t(~m_data); }
    constexpr bitboard_t operator|(const bitboard_t other) const { return bitboard_t(m_data | other.m_data); }
    constexpr bitboard_t operator&(const bitboard_t other) const { return bitboard_t(m_data & other.m_data); }
    constexpr bitboard_t operator^(const bitboard_t other) const { return bitboard_t(m_data ^ other.m_data); }

    constexpr bitboard_t operator*(const bitboard_type other) const { return bitboard_t(m_data * other); }

    constexpr bitboard_t& operator|=(const bitboard_t other)
    {
        m_data |= other.m_data;
        return *this;
    }
    constexpr bitboard_t& operator&=(const bitboard_t other)
    {
        m_data &= other.m_data;
        return *this;
    }
    constexpr bitboard_t& operator^=(const bitboard_t other)
    {
        m_data ^= other.m_data;
        return *this;
    }

    constexpr bitboard_t operator<<(const int shift) const { return bitboard_t(m_data << shift); }
    constexpr bitboard_t operator>>(const int shift) const { return bitboard_t(m_data >> shift); }

    constexpr bitboard_t& operator<<=(const int shift)
    {
        m_data <<= shift;
        return *this;
    }
    constexpr bitboard_t& operator>>=(const int shift)
    {
        m_data >>= shift;
        return *this;
    }

    constexpr bool     operator==(const bitboard_t other) const { return m_data == other.m_data; }
    constexpr bool     operator!=(const bitboard_t other) const { return m_data != other.m_data; }
    constexpr explicit operator bool() const { return m_data != 0; }

    [[nodiscard]] constexpr bool is_set(const int bit) const { return (m_data >> bit) & 1ULL; }
    void                         set(const int bit) { m_data |= (1ULL << bit); }
    void                         reset(const int bit) { m_data &= ~(1ULL << bit); }
    void                         flip(const int bit) { m_data ^= (1ULL << bit); }

    static constexpr bitboard_t empty() { return bitboard_t{0}; }
    static constexpr bitboard_t full() { return ~empty(); }

    static constexpr bitboard_t file(const file_t file) { return bitboard_t{file_a << index(file)}; }
    static constexpr bitboard_t file(const square_t sq) { return bitboard_t{file_a << index(sq.file())}; }
    static constexpr bitboard_t rank(const rank_t rank) { return bitboard_t{rank_a << index(rank) * count<rank_t>}; }
    static constexpr bitboard_t rank(const square_t sq)
    {
        return bitboard_t{rank_a << index(sq.rank()) * count<rank_t>};
    }
    static constexpr bitboard_t square(const square_t sq) { return bitboard_t{1} << index(sq); }

    static constexpr bitboard_t sides() { return file(FILE_A) | file(FILE_H) | rank(RANK_1) | rank(RANK_8); };
    static constexpr bitboard_t corners() { return square(A1) | square(A8) | square(H1) | square(H8); }

    [[nodiscard]] constexpr int popcount() const { return bit::popcount(m_data); }
    [[nodiscard]] constexpr int get_lsb() const { return bit::get_lsb(m_data); }
    [[nodiscard]] constexpr int pops_lsb() { return bit::pop_lsb(m_data); }
    [[nodiscard]] constexpr int get_msb() const { return bit::get_msb(m_data); }

    static bitboard_t from_to_incl(const square_t from, const square_t to) { return s_from_to.at(from).at(to); }
    static bitboard_t from_to_excl(const square_t from, const square_t to)
    {
        return from_to_incl(from, to) & ~square(from) & ~square(to);
    }
    static bitboard_t line(const square_t from, const square_t to) { return s_lines.at(from).at(to); }

    static bool are_aligned(const square_t sq1, const square_t sq2, const square_t sq3)
    {
        return line(sq1, sq2) == line(sq2, sq3);
    }

    template <piece_type_t pc>
    static bitboard_t attacks(square_t sq, bitboard_t occupancy = empty(), color_t c = WHITE);
    static bitboard_t attacks(piece_type_t pt, square_t sq, bitboard_t occupancy = empty(), color_t c = WHITE);

    template <direction_t dir>
    static constexpr bitboard_t direction_mask();

    template <direction_t... Dirs>
    static constexpr bitboard_t shift(bitboard_t bb);

    template <direction_t... Dirs>
    static constexpr bitboard_t ray(square_t sq, bitboard_t blockers = empty(), int len = 7);

    template <piece_type_t pc>
    static constexpr bitboard_t ray(square_t sq, bitboard_t blockers = empty(), int len = 7);

    template <piece_type_t pc>
    static constexpr bitboard_t pseudo_attack(square_t sq, color_t c);

  private:

    static void init_from_to();
    static void init_pseudo_attacks();

    bitboard_type m_data;

    static enum_array<square_t, enum_array<square_t, bitboard_t>> s_from_to;
    static enum_array<square_t, enum_array<square_t, bitboard_t>> s_lines;

    static enum_array<piece_type_t, enum_array<square_t, bitboard_t>> s_piece_pseudo_attacks;
    static enum_array<color_t, enum_array<square_t, bitboard_t>>      s_pawn_pseudo_attacks;

    static constexpr bitboard_type file_a = 0x0101010101010101ULL;
    static constexpr bitboard_type rank_a = 0x00000000000000FFULL;
};

using bb = bitboard_t;

inline void bitboard_t::init()
{
    init_pseudo_attacks();
    init_from_to();
}

template <direction_t dir>
constexpr bitboard_t bitboard_t::direction_mask()
{
    if constexpr (dir == NO_DIRECTION)
        assert(0 && "invalid direction");
    else if constexpr (dir == EAST || dir == NORTH_EAST || dir == SOUTH_EAST)
        return ~file(FILE_H);
    else if constexpr (dir == WEST || dir == NORTH_WEST || dir == SOUTH_WEST)
        return ~file(FILE_A);
    else
        return full();
}

template <direction_t... Dirs>
constexpr bitboard_t bitboard_t::shift(bitboard_t bb)
{
    if constexpr (sizeof...(Dirs) == 0)
    {
        return bb;
    }
    else if constexpr (sizeof...(Dirs) == 1)
    {
        constexpr direction_t dir  = std::get<0>(std::tuple{Dirs...});
        constexpr bitboard_t  mask = direction_mask<dir>();
        return dir.m_offset > 0 ? (bb & mask) << dir.m_offset : (bb & mask) >> -dir.m_offset;
    }
    else
    {
        ((bb = shift<Dirs>(bb)), ...);
        return bb;
    }
}

template <direction_t... Dirs>
constexpr bitboard_t bitboard_t::ray(const square_t sq, const bitboard_t blockers, int len)
{
    len = len > 7 ? 7 : len;

    if (len < 0)
    {
        return ray<-Dirs...>(sq, blockers, len);
    }

    if constexpr (sizeof...(Dirs) == 1)
    {
        constexpr direction_t Dir     = std::get<0>(std::tuple{Dirs...});
        bitboard_t            attacks = empty();
        bitboard_t            bb      = shift<Dir>(square(sq));
        for (int i = 0; i < len && bb; ++i)
        {
            attacks |= bb;
            if ((bb & blockers) != empty())
                break;
            bb = shift<Dir>(bb);
        }
        return attacks;
    }
    else
    {
        return (ray<Dirs>(sq, blockers, len) | ...);
    }
}

template <piece_type_t pc>
constexpr bitboard_t bitboard_t::ray(const square_t sq, const bitboard_t blockers, int len)
{
    static_assert(pc == BISHOP || pc == ROOK);
    len = len > 7 ? 7 : len;
    if constexpr (pc == BISHOP)
        return ray<NORTH_WEST, NORTH_EAST, SOUTH_WEST, SOUTH_EAST>(sq, blockers, len);
    else if constexpr (pc == ROOK)
        return ray<NORTH, SOUTH, EAST, WEST>(sq, blockers, len);
    else
        return empty();
}

template <piece_type_t pc>
constexpr bitboard_t bitboard_t::pseudo_attack(const square_t sq, const color_t c)
{
    if constexpr (pc == PAWN)
        return s_pawn_pseudo_attacks.at(c).at(sq);
    else if constexpr (pc == KNIGHT || pc == BISHOP || pc == ROOK || pc == QUEEN || pc == KING)
        return s_piece_pseudo_attacks.at(pc).at(sq);
    else
        return empty();
}

template <piece_type_t pc>
constexpr bitboard_t relevancy_mask(square_t sq)
{
    bitboard_t mask = ~bb::sides();
    if constexpr (pc == ROOK)
    {
        if (sq.rank() == RANK_1 || sq.rank() == RANK_8)
        {
            mask |= bb::rank(sq);
        }
        if (sq.file() == FILE_A || sq.file() == FILE_H)
        {
            mask |= bb::file(sq);
        }
        mask &= ~bb::corners();
    }
    return bb::ray<pc>(sq) & mask;
}

template <piece_type_t pc>
constexpr std::size_t compute_magic_sz()
{
    size_t ret = 0;
    for (auto sq = A1; sq <= H8; ++sq)
    {
        ret += 1ULL << relevancy_mask<pc>(sq).popcount();
    }
    return ret;
}

template <piece_type_t pc>
struct magics_t
{
    struct magic_val_t
    {
        using shift_type = int32_t;
        using index_type = uint32_t;
        using magic_type = bitboard_t::bitboard_type;
        using mask_type = bitboard_t;



        mask_type                mask;
        magic_type                magic;
        shift_type                shift;
        index_type                offset;

        [[nodiscard]] index_type index(const bitboard_t blockers) const
        {
            return offset + (((blockers & mask).value() * magic) >> shift);
        }
    };
    static constexpr std::size_t sz = compute_magic_sz<pc>();

    magics_t();
    [[nodiscard]] bitboard_t attack(const square_t sq, const bitboard_t occupancy) const
    {
        return attacks.at(magic_vals.at(sq).index(occupancy));
    }

    enum_array<square_t, magic_val_t> magic_vals = {};
    std::array<bitboard_t, sz>        attacks    = {};
};

using rook_magics   = magics_t<ROOK>;
using bishop_magics = magics_t<BISHOP>;

template <piece_type_t pc>
extern magics_t<pc> g_magics;

template <piece_type_t pc>
bitboard_t bitboard_t::attacks(const square_t sq, const bitboard_t occupancy, const color_t c)
{
    if constexpr (pc == ROOK || pc == BISHOP)
    {
        return g_magics<pc>.attack(sq, occupancy);
    }
    else if constexpr (pc == QUEEN)
    {
        return attacks<BISHOP>(sq, occupancy) | attacks<ROOK>(sq, occupancy);
    }
    else if constexpr (pc == PAWN || pc == KNIGHT || pc == KING)
    {
        return bb::pseudo_attack<pc>(sq, c);
    }
    else
    {
        assert(false && "invalid piece type");
        return bb::empty();
    }
}
inline bitboard_t bitboard_t::attacks(const piece_type_t pt, const square_t sq, const bitboard_t occupancy,
                                      const color_t c)
{
    switch (index(pt))
    {
        case index(PAWN):
            return attacks<PAWN>(sq, occupancy, c);
        case index(KNIGHT):
            return attacks<KNIGHT>(sq, occupancy, c);
        case index(BISHOP):
            return attacks<BISHOP>(sq, occupancy, c);
        case index(ROOK):
            return attacks<ROOK>(sq, occupancy, c);
        case index(QUEEN):
            return attacks<QUEEN>(sq, occupancy, c);
        case index(KING):
            return attacks<KING>(sq, occupancy, c);
        default:
            assert(0 && "invalid piece type");
            return bb::empty();
    }
}

#endif
