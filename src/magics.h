#ifndef MAGICS_H_INCLUDED
#define MAGICS_H_INCLUDED
#include "bitboard.h"

struct magic_val_t
{
    bitboard_t mask;
    uint64_t   shift;
    bitboard_t magic;
    size_t     offset;
    bitboard_t index(const bitboard_t blockers) const
    {
        return offset + (((blockers & mask) * magic) >> shift);
    }
};

template <piece_type_t pc>
struct magics_t
{
    static constexpr size_t compute_magic_sz();
    static constexpr size_t attacks_sz = compute_magic_sz();

    magics_t();
    bitboard_t attack(const square_t sq, const bitboard_t occupancy) const;

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
    for (square_t sq = A1; sq < NB_SQUARES; sq++)
    {
        ret += 1ULL << (popcount(relevancy_mask<pc>(sq)));
    }
    return ret;
}

template struct magics_t<ROOK>;
template struct magics_t<BISHOP>;

template <piece_type_t pc>
extern magics_t<pc> magics;

namespace Bitboard
{
    template <piece_type_t pc>
    bitboard_t attacks(const square_t sq, const bitboard_t occupancy, const color_t c = WHITE)
    {
        if constexpr (pc == ROOK || pc == BISHOP)
        {
            return magics<pc>.attack(sq, occupancy);
        }
        else if constexpr (pc == QUEEN)
        {
            return attacks<BISHOP>(sq, occupancy) | attacks<ROOK>(sq, occupancy);
        }
        else
        {
            return pseudo_attack<pc>(sq, c);
        }
    }
} // namespace Bitboard

#endif
