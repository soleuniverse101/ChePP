#include "ChePP/engine/bitboard.h"

#include <random>
#include <sstream>

enum_array<square_t, enum_array<square_t, bitboard_t>>     bb::s_from_to{};
enum_array<square_t, enum_array<square_t, bitboard_t>>     bb::s_lines{};
enum_array<piece_type_t, enum_array<square_t, bitboard_t>> bb::s_piece_pseudo_attacks{};
enum_array<color_t, enum_array<square_t, bitboard_t>>      bb::s_pawn_pseudo_attacks{};

template <>
magics_t<BISHOP> g_magics<BISHOP>;
template <>
magics_t<ROOK> g_magics<ROOK>;

void bitboard_t::init_from_to()
{
    for (auto sq1 = A1; sq1 <= H8; ++sq1)
    {
        for (auto sq2 = A1; sq2 <= H8; ++sq2)
        {

            if (attacks<ROOK>(sq1) & square(sq2))
            {
                s_lines.at(sq1).at(sq2)   = attacks<ROOK>(sq1) & attacks<ROOK>(sq2);
                s_from_to.at(sq1).at(sq2) = attacks<ROOK>(sq1, square(sq2)) & attacks<ROOK>(sq2, square(sq1));
                s_lines.at(sq1).at(sq2) |= (square(sq1) | square(sq2));
                s_from_to.at(sq1).at(sq2) |= (square(sq1) | square(sq2));
            }
            if (attacks<BISHOP>(sq1) & square(sq2))
            {
                s_lines.at(sq1).at(sq2)   = attacks<BISHOP>(sq1) & attacks<BISHOP>(sq2);
                s_from_to.at(sq1).at(sq2) = attacks<BISHOP>(sq1, square(sq2)) & attacks<BISHOP>(sq2, square(sq1));
                s_lines.at(sq1).at(sq2) |= (square(sq1) | square(sq2));
                s_from_to.at(sq1).at(sq2) |= (square(sq1) | square(sq2));
            }
        }
    }
}

void bitboard_t::init_pseudo_attacks()
{
    for (auto sq = A1; sq <= H8; ++sq)
    {
        const bitboard_t bb                      = square(sq);
        s_pawn_pseudo_attacks.at(WHITE).at(sq)   = shift<NORTH_WEST>(bb) | shift<NORTH_EAST>(bb);
        s_pawn_pseudo_attacks.at(BLACK).at(sq)   = shift<SOUTH_WEST>(bb) | shift<SOUTH_EAST>(bb);
        s_piece_pseudo_attacks.at(KNIGHT).at(sq) = shift<NORTH, NORTH, EAST>(bb) | shift<NORTH, NORTH, WEST>(bb) |
                                                   shift<SOUTH, SOUTH, EAST>(bb) | shift<SOUTH, SOUTH, WEST>(bb) |
                                                   shift<EAST, EAST, NORTH>(bb) | shift<EAST, EAST, SOUTH>(bb) |
                                                   shift<WEST, WEST, NORTH>(bb) | shift<WEST, WEST, SOUTH>(bb);
        s_piece_pseudo_attacks.at(BISHOP).at(sq) = ray<BISHOP>(sq);
        s_piece_pseudo_attacks.at(ROOK).at(sq)   = ray<ROOK>(sq);
        s_piece_pseudo_attacks.at(QUEEN).at(sq)  = ray<BISHOP>(sq) | ray<ROOK>(sq);
        s_piece_pseudo_attacks.at(KING).at(sq)   = shift<NORTH>(bb) | shift<SOUTH>(bb) | shift<EAST>(bb) |
                                                 shift<WEST>(bb) | shift<NORTH, EAST>(bb) | shift<NORTH, WEST>(bb) |
                                                 shift<SOUTH, EAST>(bb) | shift<SOUTH, WEST>(bb);
    }
}

static uint64_t random_u64()
{
    static std::random_device                      rd;
    static std::mt19937                            gen(rd());
    static std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    return dist(gen);
}
static uint64_t random_magic()
{
    return random_u64() & random_u64() & random_u64();
}

constexpr bitboard_t mask_nb(const bitboard_t mask, const uint64_t n)
{
    bitboard_t bb{0};
    int        idx = 0;
    for (auto sq = A1; sq <= H8; ++sq)
    {
        if (bb::square(sq) & mask)
        {
            if (bitboard_t{1} << idx & bitboard_t{n})
            {
                bb |= bb::square(sq);
            }
            idx++;
        }
    }
    assert((n & ~((1ULL << idx) - 1)) == 0 && "index of msb of n (little endian) must be lesser than popcnt(mask)");
    return bb;
}

// we can rely on constructor because there is no dependency on other global variable
// beware if such dependecies are introduced we have no guarantee on initialisation order
template <piece_type_t pc>
magics_t<pc>::magics_t()
{
    constexpr uint64_t MAX_TRIES{UINT64_MAX};
    constexpr uint64_t MAX_COMB{4096};

    std::array<bitboard_t, MAX_COMB> cached_blockers{};
    std::array<bitboard_t, MAX_COMB> cached_attacks{};

    typename magic_val_t::index_type offset{0};

    for (auto sq = A1; sq <= H8; ++sq)
    {
        const typename magic_val_t::mask_type  mask{relevancy_mask<pc>(sq)};
        const int                              nb_ones{mask.popcount()};
        const int                              combinations{1 << nb_ones};
        const typename magic_val_t::shift_type shift{64 - nb_ones};
        typename magic_val_t::magic_type       magic{0};

        assert(combinations <= MAX_COMB && "to many blockers variations, check relevancy mask");

        for (uint64_t comb = 0; comb < combinations; comb++)
        {
            cached_blockers.at(comb) = mask_nb(mask, comb);
            cached_attacks.at(comb)  = bb::ray<pc>(sq, cached_blockers.at(comb));
        }

        for (uint64_t tries = 1;; tries++)
        {
            std::array<bool, MAX_COMB> tries_map{};
            bool                       fail{false};

            magic = random_magic();

            for (uint64_t c = 0; c < combinations; c++)
            {
                const typename magic_val_t::index_type index = (cached_blockers.at(c).value() * magic) >> shift;

                if (const bitboard_t cur = attacks.at(offset + index);
                    tries_map.at(index) && (cur != cached_attacks.at(c)))
                {
                    fail = true;
                    break;
                }

                attacks.at(offset + index) = cached_attacks.at(c);
                tries_map.at(index)        = true;
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

std::string bitboard_t::to_string() const
{
    std::ostringstream out;
    out << "__________________\n";
    for (auto rank = RANK_1; rank <= RANK_8; ++rank)
    {
        out << "|";
        for (auto file  = FILE_A; file <= FILE_H; ++file)
        {

            out << (is_set(index(square_t{{file, RANK_8 - rank}})) ? "X " : ". ");
        }
        out << "|\n";
    }
    out << "__________________\n";
    return out.str();
}
