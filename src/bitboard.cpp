#include "bitboard.h"
#include "cstring"

#include <random>
#include <sstream>

using namespace Bitboard;

all_squares<all_squares<bitboard_t>>     bb::g_from_to{};
all_squares<all_squares<bitboard_t>>     bb::g_lines{};
all_piece_types<all_squares<bitboard_t>> bb::g_piece_pseudo_attacks{};
all_colors<all_squares<bitboard_t>>      bb::g_pawn_pseudo_attacks{};
template <>
magics_t<BISHOP> g_magics<BISHOP>;
template <>
magics_t<ROOK> g_magics<ROOK>;

void init_from_to()
{
    for (square_t sq1 = A1; sq1 < NB_SQUARES; sq1 = static_cast<square_t>(sq1 + 1))
    {
        for (square_t sq2 = A1; sq2 < NB_SQUARES; sq2 = static_cast<square_t>(sq2 + 1))
        {

            if (attacks<ROOK>(sq1) & sq_mask(sq2))
            {
                g_lines.at(sq1).at(sq2) = attacks<ROOK>(sq1) & attacks<ROOK>(sq2);
                g_from_to.at(sq1).at(sq2) =
                    attacks<ROOK>(sq1, sq_mask(sq2)) & attacks<ROOK>(sq2, sq_mask(sq1));
                g_lines.at(sq1).at(sq2) |= (sq_mask(sq1) | sq_mask(sq2));
                g_from_to.at(sq1).at(sq2) |= (sq_mask(sq1) | sq_mask(sq2));
            }
            if (attacks<BISHOP>(sq1) & sq_mask(sq2))
            {
                g_lines.at(sq1).at(sq2) = attacks<BISHOP>(sq1) & attacks<BISHOP>(sq2);
                g_from_to.at(sq1).at(sq2) =
                    attacks<BISHOP>(sq1, sq_mask(sq2)) & attacks<BISHOP>(sq2, sq_mask(sq1));
                g_lines.at(sq1).at(sq2) |= (sq_mask(sq1) | sq_mask(sq2));
                g_from_to.at(sq1).at(sq2) |= (sq_mask(sq1) | sq_mask(sq2));
            }

        }
    }
}

void init_pseudo_attacks()
{
    for (square_t sq = A1; sq < NB_SQUARES; sq = static_cast<square_t>(sq + 1))
    {
        const bitboard_t bb                      = sq_mask(sq);
        g_pawn_pseudo_attacks.at(WHITE).at(sq) = shift<NORTH_WEST>(bb) | shift<NORTH_EAST>(bb);
        g_pawn_pseudo_attacks.at(BLACK).at(sq) = shift<SOUTH_WEST>(bb) | shift<SOUTH_EAST>(bb);
        g_piece_pseudo_attacks.at(KNIGHT).at(sq) =
            shift<NORTH, NORTH, EAST>(bb) | shift<NORTH, NORTH, WEST>(bb) |
            shift<SOUTH, SOUTH, EAST>(bb) | shift<SOUTH, SOUTH, WEST>(bb) |
            shift<EAST, EAST, NORTH>(bb) | shift<EAST, EAST, SOUTH>(bb) |
            shift<WEST, WEST, NORTH>(bb) | shift<WEST, WEST, SOUTH>(bb);
        g_piece_pseudo_attacks.at(BISHOP).at(sq) = ray<BISHOP>(sq);
        g_piece_pseudo_attacks.at(ROOK).at(sq)   = ray<ROOK>(sq);
        g_piece_pseudo_attacks.at(QUEEN).at(sq)  = ray<BISHOP>(sq) | ray<ROOK>(sq);
        g_piece_pseudo_attacks.at(KING).at(sq)   = shift<NORTH>(bb) | shift<SOUTH>(bb) |
                                                   shift<EAST>(bb) | shift<WEST>(bb) |
                                                   shift<NORTH, EAST>(bb) | shift<NORTH, WEST>(bb) |
                                                   shift<SOUTH, EAST>(bb) | shift<SOUTH, WEST>(bb);
    }
}

void Bitboard::init()
{
    init_pseudo_attacks();
    init_from_to();
}
static uint64_t random_u64()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    return dist(gen);
}
static uint64_t random_magic()
{
    return random_u64() & random_u64() & random_u64();
}

static bitboard_t mask_nb(const bitboard_t mask, const uint64_t n)
{
    bitboard_t bb  = 0;
    int        idx = 0;
    for (square_t sq = A1; sq < NB_SQUARES; sq = static_cast<square_t>(sq + 1))
    {
        if (sq_mask(sq) & mask)
        {
            if (static_cast<bitboard_t>(1) << idx & n)
            {
                bb |= sq_mask(sq);
            }
            idx++;
        }
    }
    assert((n & ~((1ULL << idx) - 1)) == 0 &&
           "index of msb of n (little endian) must be lesser than popcnt(mask)");
    return bb;
}

// we can rely on constructor because there is no dependency on other global variable
// beware if such dependecies are introduced we have no guarantee on initialisation order
template <piece_type_t pc>
magics_t<pc>::magics_t()
{
    constexpr uint64_t MAX_TRIES = UINT64_MAX;
    constexpr uint64_t MAX_COMB  = 4096;

    std::array<bitboard_t, MAX_COMB> cached_blockers{};
    std::array<bitboard_t, MAX_COMB> cached_attacks{};

    size_t offset = 0;

    for (square_t sq = A1; sq < NB_SQUARES; sq = static_cast<square_t>(sq + 1))
    {
        const bitboard_t mask         = relevancy_mask<pc>(sq);
        const int        nb_ones      = popcount(mask);
        const uint64_t   combinations = 1ULL << nb_ones;
        const size_t        shift      = 64 - nb_ones;
        bitboard_t       magic        = 0;

        assert(combinations <= MAX_COMB && "to many blockers variations, check relevancy mask");

        for (uint64_t comb = 0; comb < combinations; comb++)
        {
            cached_blockers.at(comb) = mask_nb(mask, comb);
            cached_attacks.at(comb)  = bb::ray<pc>(sq, cached_blockers.at(comb));
        }

        for (uint64_t tries = 1;; tries++)
        {
            std::array<bool, MAX_COMB> tries_map{};
            bool                       fail = false;

            magic = random_magic();

            for (uint64_t c = 0; c < combinations; c++)
            {
                const size_t     index = (cached_blockers.at(c) * magic) >> shift;

                if (const bitboard_t cur = attacks.at(offset + index);
                    tries_map.at(index) && (cur != cached_attacks.at(c)))
                {
                    fail = true;
                    break;
                }

                attacks.at(offset + index) = cached_attacks.at(c);
                tries_map.at(index)              = true;
            }

            if (!fail)
            {
                break;
            }

            if (tries == MAX_TRIES - 1)
            {
                assert(0 && "failed to find magic");
            }
        }

        g_magics<pc>.magic_vals.at(sq).offset = offset;
        g_magics<pc>.magic_vals.at(sq).magic  = magic;
        g_magics<pc>.magic_vals.at(sq).mask   = mask;
        g_magics<pc>.magic_vals.at(sq).shift  = shift;

        offset += combinations;
    }
}

std::string Bitboard::string(const bitboard_t bb)
{
    std::ostringstream out;
    out << "__________________\n";
    for (rank_t rank = RANK_1; rank <= RANK_8; rank = static_cast<rank_t>(rank + 1))
    {
        out << "|";
        for (file_t file = FILE_A; file <= FILE_H; file = static_cast<file_t>(file + 1))
        {
            out << ((bb & sq_mask(static_cast<square_t>((NB_RANKS - rank - 1) * 8 + file))) ? "X " : ". ");
        }
        out << "|\n";
    }
    out << "__________________\n";
    return out.str();
}
