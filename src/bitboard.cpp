#include "bitboard.h"
#include "cstring"
#include <sstream>

using namespace Bitboard;

all_squares<all_squares<bitboard_t>>     bb::from_to{};
all_squares<all_squares<bitboard_t>>     bb::line{};
all_piece_types<all_squares<bitboard_t>> bb::piece_pseudo_attacks{};
all_colors<all_squares<bitboard_t>>      bb::pawn_pseudo_attacks{};
template <>
magics_t<BISHOP> magics<BISHOP>;
template <>
magics_t<ROOK> magics<ROOK>;

void init_from_to()
{
    for (square_t sq1 = A1; sq1 < NB_SQUARES; ++sq1)
    {
        for (square_t sq2 = A1; sq2 < NB_SQUARES; ++sq2)
        {
            const int df = fl_of(sq2) - fl_of(sq1);
            const int dr = rk_of(sq2) - rk_of(sq1);
            if (dr == 0) // same file
            {
                from_to.at(sq1).at(sq2) = ray<EAST>(sq1, 0, df);
            }
            if (df == 0) // same rank
            {
                from_to.at(sq1).at(sq2) = ray<NORTH>(sq1, 0, dr);
            }
            if (df == dr) // diagonal
            {
                from_to.at(sq1).at(sq2) =
                    dr < 0 ? (df < 0 ? ray<SOUTH_WEST>(sq1, 0, -df) : ray<SOUTH_EAST>(sq1, 0, df))
                           : (df < 0 ? ray<NORTH_WEST>(sq1, 0, -df) : ray<NORTH_EAST>(sq1, 0, df));
            }
            from_to.at(sq1).at(sq2) |= (sq_mask(sq1) | sq_mask(sq2));
        }
    }
}

void init_pseudo_attacks()
{
    for (square_t sq = A1; sq < NB_SQUARES; ++sq)
    {
        const bitboard_t bb                      = bb::sq_mask(sq);
        bb::pawn_pseudo_attacks.at(WHITE).at(sq) = shift<NORTH_WEST>(bb) | shift<NORTH_EAST>(bb);
        bb::pawn_pseudo_attacks.at(BLACK).at(sq) = shift<SOUTH_WEST>(bb) | shift<SOUTH_EAST>(bb);
        bb::piece_pseudo_attacks.at(KNIGHT).at(sq) =
            shift<NORTH, NORTH, EAST>(bb) | shift<NORTH, NORTH, WEST>(bb) |
            shift<SOUTH, SOUTH, EAST>(bb) | shift<SOUTH, SOUTH, WEST>(bb) |
            shift<EAST, EAST, NORTH>(bb) | shift<EAST, EAST, SOUTH>(bb) |
            shift<WEST, WEST, NORTH>(bb) | shift<WEST, WEST, SOUTH>(bb);
        bb::piece_pseudo_attacks.at(BISHOP).at(sq) = ray<BISHOP>(sq);
        bb::piece_pseudo_attacks.at(ROOK).at(sq)   = ray<ROOK>(sq);
        bb::piece_pseudo_attacks.at(QUEEN).at(sq)  = ray<BISHOP>(sq) | ray<ROOK>(sq);
        bb::piece_pseudo_attacks.at(KING).at(sq)   = shift<NORTH>(bb) | shift<SOUTH>(bb) |
                                                   shift<EAST>(bb) | shift<WEST>(bb) |
                                                   shift<NORTH, EAST>(bb) | shift<NORTH, WEST>(bb) |
                                                   shift<SOUTH, EAST>(bb) | shift<SOUTH, WEST>(bb);
    }
}

void Bitboard::init()
{
    init_pseudo_attacks();
}
static uint64_t random_u64()
{
    return rand() + (((uint64_t)rand()) << 32);
}
static uint64_t random_magic()
{
    return random_u64() & random_u64() & random_u64();
}

static bitboard_t mask_nb(const bitboard_t mask, const uint64_t n)
{
    bitboard_t bb  = 0;
    int        idx = 0;
    for (square_t sq = A1; sq < NB_SQUARES; sq++)
    {
        if (bb::sq_mask(sq) & mask)
        {
            if (1 << idx & n)
            {
                bb |= bb::sq_mask(sq);
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

    std::array<bitboard_t, MAX_COMB> blockers;
    std::array<bitboard_t, MAX_COMB> attacks;

    size_t offset = 0;

    for (square_t sq = A1; sq < NB_SQUARES; ++sq)
    {
        const bitboard_t mask         = relevancy_mask<pc>(sq);
        const int        nb_ones      = popcount(mask);
        const uint64_t   combinations = 1ULL << nb_ones;
        const int        shift        = 64 - nb_ones;
        bitboard_t       magic        = 0;

        assert(combinations <= MAX_COMB && "to many blockers variations, check relevancy mask");

        for (uint64_t comb = 0; comb < combinations; comb++)
        {
            blockers.at(comb) = mask_nb(mask, comb);
            attacks.at(comb)  = bb::ray<pc>(sq, blockers.at(comb));
        }

        for (uint64_t tries = 1;; tries++)
        {
            std::array<bool, MAX_COMB> tries_map{};
            bool                       fail = false;

            magic = random_magic();

            for (uint64_t c = 0; c < combinations; c++)
            {
                const size_t     index = (blockers.at(c) * magic) >> shift;
                const bitboard_t cur   = this->attacks.at(offset + index);

                if (tries_map.at(index) && (cur != attacks.at(c)))
                {
                    fail = true;
                    break;
                }

                this->attacks.at(offset + index) = attacks.at(c);
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

        magics<pc>.magic_vals.at(sq).offset = offset;
        magics<pc>.magic_vals.at(sq).magic  = magic;
        magics<pc>.magic_vals.at(sq).mask   = mask;
        magics<pc>.magic_vals.at(sq).shift  = shift;

        offset += combinations;
    }
}

std::string Bitboard::string(bitboard_t bb)
{
    std::ostringstream out;
    out << "__________________\n";
    for (rank_t rank = RANK_1; rank <= RANK_8; ++rank)
    {
        out << "|";
        for (file_t file = FILE_A; file <= FILE_H; ++file)
        {
            out << ((bb & sq_mask((square_t)((NB_RANKS - rank - 1) * 8 + file))) ? "X " : ". ");
        }
        out << "|\n";
    }
    out << "__________________\n";
    return out.str();
}
