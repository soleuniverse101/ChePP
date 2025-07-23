#include "magics.h"
#include "chrono"

template <>
magics_t<BISHOP> magics<BISHOP>;
template <>
magics_t<ROOK> magics<ROOK>;

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

int main(void)
{
    bb::init();
    std::cout << Bitboard::string(bb::attacks<QUEEN>(D5, bb::sq_mask(E6)));
    std::cout << Bitboard::string(bb::attacks<ROOK>(H8, bb::attacks<QUEEN>(D5, 0)));
    std::cout << Bitboard::string(bb::attacks<KNIGHT>(D5, bb::attacks<QUEEN>(D5, 0)));
    std::cout << Bitboard::string(bb::attacks<PAWN>(H4, bb::attacks<QUEEN>(D5, 0)));
    const int           iterations = 1'000'000;
    volatile bitboard_t total      = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        for (int sq = 0; sq < 64; ++sq)
        {
            total ^= bb::attacks<QUEEN>((square_t)sq, i);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Movegen benchmark (QUEEN, " << iterations << "x64): " << elapsed.count() << "s\n";
    std::cout << "Total (to avoid opt): " << total << "\n";
}
