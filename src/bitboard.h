#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED
#include "types.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <string>

namespace Bitboard
{
    constexpr bitboard_t empty  = 0ULL;
    constexpr bitboard_t full   = ~empty;
    constexpr bitboard_t file_a = 0x0101010101010101ULL;
    constexpr bitboard_t rank_a = 0x00000000000000FFULL;

    constexpr bitboard_t fl_mask(const file_t file)
    {
        return file_a << static_cast<int8_t>(file);
    }

    constexpr bitboard_t rk_mask(const rank_t rank)
    {
        return rank_a << (static_cast<int8_t>(rank) * 8);
    }

    constexpr bitboard_t sq_mask(const square_t sq)
    {
        return static_cast<bitboard_t>(1) << static_cast<int8_t>(sq);
    }

    constexpr bitboard_t fl_mask(const square_t sq)
    {
        return fl_mask(fl_of(sq));
    }

    constexpr bitboard_t rk_mask(const square_t sq)
    {
        return rk_mask(rk_of(sq));
    }

    constexpr bitboard_t sides =
        fl_mask(FILE_A) | fl_mask(FILE_H) | rk_mask(RANK_1) | rk_mask(RANK_8);
    constexpr bitboard_t corners = sq_mask(A1) | sq_mask(A8) | sq_mask(H1) | sq_mask(H8);

    extern all_squares<all_squares<bitboard_t>> g_from_to;
    extern all_squares<all_squares<bitboard_t>> g_lines;

    inline bitboard_t from_to_incl(const square_t from, const square_t to)
    {
        return g_from_to.at(from).at(to);
    }
    inline bitboard_t from_to_excl(const square_t from, const square_t to)
    {
        return from_to_incl(from, to) & ~sq_mask(from) & ~sq_mask(to);
    }
    inline bitboard_t line(const square_t from, const square_t to)
    {
        return g_lines.at(from).at(to);
    }

    template <direction_t dir>
    constexpr bitboard_t direction_mask()
    {
        switch (dir)
        {
            case EAST:
            case NORTH_EAST:
            case SOUTH_EAST:
                return ~fl_mask(FILE_H);
            case WEST:
            case NORTH_WEST:
            case SOUTH_WEST:
                return ~fl_mask(FILE_A);
            default:
                return full;
        }
    }

    template <direction_t... Dirs>
    constexpr bitboard_t shift(bitboard_t bb)
    {
        if constexpr (sizeof...(Dirs) == 0)
        {
            return bb;
        }
        else if constexpr (sizeof...(Dirs) == 1)
        {
            constexpr direction_t dir  = std::get<0>(std::tuple{Dirs...});
            constexpr bitboard_t  mask = direction_mask<dir>();
            return dir > 0 ? (bb & mask) << dir : (bb & mask) >> -dir;
        }
        else
        {
            ((bb = shift<Dirs>(bb)), ...);
            return bb;
        }
    }

    template <direction_t... Dirs>
    constexpr bitboard_t ray(const square_t sq, const bitboard_t blockers = 0, int len = 7)
    {
        len = len > 7 ? 7 : len;

        if (len < 0)
        {
            return ray<inverse_dir<Dirs>()...>(sq, blockers, -len);
        }

        if constexpr (sizeof...(Dirs) == 1)
        {
            constexpr direction_t Dir     = std::get<0>(std::tuple{Dirs...});
            bitboard_t            attacks = 0;
            bitboard_t            bb      = shift<Dir>(sq_mask(sq));
            for (int i = 0; i < len && bb; ++i)
            {
                attacks |= bb;
                if (bb & blockers)
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
    constexpr bitboard_t ray(const square_t sq, const bitboard_t blockers = 0, int len = 7)
    {
        static_assert(pc == BISHOP || pc == ROOK);
        len = len > 7 ? 7 : len;
        return pc == BISHOP ? ray<NORTH_WEST, NORTH_EAST, SOUTH_WEST, SOUTH_EAST>(sq, blockers, len)
                            : ray<NORTH, SOUTH, EAST, WEST>(sq, blockers, len);
    }

    extern all_piece_types<all_squares<bitboard_t>> g_piece_pseudo_attacks;
    extern all_colors<all_squares<bitboard_t>>      g_pawn_pseudo_attacks;

    // Returns the base attack of a piece at a square sq. Base attacks assume an empty board
    template <piece_type_t pc>
    constexpr bitboard_t pseudo_attack(const square_t sq, const color_t c)
    {
        return (pc == PAWN) ? g_pawn_pseudo_attacks.at(c).at(sq) : g_piece_pseudo_attacks.at(pc).at(sq);
    }

    void init();
    std::string string(bitboard_t bb);
} // namespace Bitboard
namespace bb = Bitboard;

struct magic_val_t
{
    bitboard_t  mask;
    uint64_t    shift;
    bitboard_t  magic;
    std::size_t offset;
    [[nodiscard]] bitboard_t  index(const bitboard_t blockers) const
    {
        return offset + (((blockers & mask) * magic) >> shift);
    }
};

template <piece_type_t pc>
struct magics_t
{
    static constexpr std::size_t compute_magic_sz();
    static constexpr std::size_t attacks_sz = compute_magic_sz();

    magics_t();
    [[nodiscard]] bitboard_t attack(square_t sq, bitboard_t occupancy) const;

    all_squares<magic_val_t>           magic_vals = {};
    std::array<bitboard_t, attacks_sz> attacks    = {};
};

template <piece_type_t pc>
constexpr bitboard_t relevancy_mask(const square_t sq)
{
    bitboard_t mask = ~bb::sides;
    if (pc == ROOK)
    {
        if (rk_of(sq) == RANK_1 || rk_of(sq) == RANK_8)
        {
            mask |= bb::rk_mask(sq);
        }
        if (fl_of(sq) == FILE_A || fl_of(sq) == FILE_H)
        {
            mask |= bb::fl_mask(sq);
        }
        mask &= ~bb::corners;
    }
    return bb::ray<pc>(sq) & mask;
}

template <piece_type_t pc>
bitboard_t magics_t<pc>::attack(const square_t sq, const bitboard_t occupancy) const
{
    return attacks.at(magic_vals.at(sq).index(occupancy));
}

template <piece_type_t pc>
constexpr size_t magics_t<pc>::compute_magic_sz()
{
    size_t ret = 0;
    for (square_t sq = A1; sq < NB_SQUARES; sq = static_cast<square_t>(sq + 1))
    {
        ret += 1ULL << (popcount(relevancy_mask<pc>(sq)));
    }
    return ret;
}

template struct magics_t<ROOK>;
template struct magics_t<BISHOP>;

template <piece_type_t pc>
extern magics_t<pc> g_magics;

namespace Bitboard
{
    template <piece_type_t pc>
    constexpr bitboard_t attacks(const square_t sq, const bitboard_t occupancy = empty,
                                 const color_t c = WHITE)
    {
        if constexpr (pc == ROOK || pc == BISHOP)
        {
            return g_magics<pc>.attack(sq, occupancy);
        }
        else if constexpr (pc == QUEEN)
        {
            return attacks<BISHOP>(sq, occupancy, c) | attacks<ROOK>(sq, occupancy, c);
        }
        else
        {
            return pseudo_attack<pc>(sq, c);
        }
    }
} // namespace Bitboard
#endif
