#include "bitboard.h"
#include "cstring"
#include <sstream>

bitboard_t bb_from_to[NB_SQUARES][NB_SQUARES];
bitboard_t bb_piece_base_attack[NB_PIECES - KNIGHT][NB_SQUARES];
bitboard_t bb_pawn_base_attack[NB_COLORS][NB_SQUARES];
bitboard_t bishop_attacks[bishop_attacks_sz];
bitboard_t rook_attacks[rook_attacks_sz];
magic_t    bishop_magic[NB_SQUARES];
magic_t    rook_magics[NB_SQUARES];

void init_bb_from_to()
{
    for (square_t sq1 = A1; sq1 < NB_SQUARES; ++sq1)
    {
        for (square_t sq2 = A1; sq2 < NB_SQUARES; ++sq2)
        {
            int df = file_of(sq2) - file_of(sq1);
            int dr = rank_of(sq2) - rank_of(sq1);
            if (dr == 0) // same file
            {
                bb_from_to[sq1][sq2] = df < 0 ? sliding_attack_bb<WEST>(sq1, 0, -df)
                                              : sliding_attack_bb<EAST>(sq1, 0, df);
            }
            if (df == 0) // same rank
            {
                bb_from_to[sq1][sq2] = dr < 0 ? sliding_attack_bb<SOUTH>(sq1, 0, -dr)
                                              : sliding_attack_bb<NORTH>(sq1, 0, dr);
            }
            if (df == dr) // diagonal
            {
                bb_from_to[sq1][sq2] = dr < 0
                                           ? (df < 0 ? sliding_attack_bb<SOUTH_WEST>(sq1, 0, -df)
                                                     : sliding_attack_bb<SOUTH_EAST>(sq1, 0, df))
                                           : (df < 0 ? sliding_attack_bb<NORTH_WEST>(sq1, 0, -df)
                                                     : sliding_attack_bb<NORTH_EAST>(sq1, 0, df));
            }
            bb_from_to[sq1][sq2] |= square_to_bb(sq1);
        }
    }
}

void init_bb_base_attacks()
{
    for (square_t sq = A1; sq < NB_SQUARES; ++sq)
    {
        bitboard_t bb                  = square_to_bb(sq);
        bb_pawn_base_attack[WHITE][sq] = move_dir_bb<NORTH_WEST>(bb) | move_dir_bb<NORTH_EAST>(bb);
        bb_pawn_base_attack[BLACK][sq] = move_dir_bb<SOUTH_WEST>(bb) | move_dir_bb<SOUTH_EAST>(bb);
        bb_piece_base_attack[KNIGHT - KNIGHT][sq] =
            move_dir_bb<NORTH, NORTH, EAST>(bb) | move_dir_bb<NORTH, NORTH, WEST>(bb) |
            move_dir_bb<SOUTH, SOUTH, EAST>(bb) | move_dir_bb<SOUTH, SOUTH, WEST>(bb) |
            move_dir_bb<EAST, EAST, NORTH>(bb) | move_dir_bb<EAST, EAST, SOUTH>(bb) |
            move_dir_bb<WEST, WEST, NORTH>(bb) | move_dir_bb<WEST, WEST, SOUTH>(bb);
        bb_piece_base_attack[BISHOP - KNIGHT][sq] = sliding_attack_bb<BISHOP>(sq);
        bb_piece_base_attack[ROOK - KNIGHT][sq]   = sliding_attack_bb<ROOK>(sq);
        bb_piece_base_attack[QUEEN - KNIGHT][sq] =
            bb_piece_base_attack[BISHOP - KNIGHT][sq] | bb_piece_base_attack[ROOK - KNIGHT][sq];
        bb_piece_base_attack[KING - KNIGHT][sq] =
            move_dir_bb<NORTH>(bb) | move_dir_bb<SOUTH>(bb) | move_dir_bb<EAST>(bb) |
            move_dir_bb<WEST>(bb) | move_dir_bb<NORTH, EAST>(bb) | move_dir_bb<NORTH, WEST>(bb) |
            move_dir_bb<SOUTH, EAST>(bb) | move_dir_bb<SOUTH, WEST>(bb);
    }
}

// returns the mask with the the bits set to the corresponding bit positions in n
// useful for generating all blocker combinations
static bitboard_t bb_from_mask(bitboard_t mask, int nb_ones, int n)
{
    assert(1ULL << nb_ones >= n);
    bitboard_t bb  = 0;
    int        idx = 0;
    for (square_t sq = A1; sq < NB_SQUARES; sq++)
    {
        if (square_to_bb(sq) & mask)
        {
            if (1 << idx & n)
            {
                bb |= square_to_bb(sq);
            }
            idx++;
        }
    }
    return bb;
}

static uint64_t random_u64()
{
    return rand() + (((uint64_t)rand()) << 32);
}
static uint64_t random_magic()
{
    return random_u64() & random_u64() & random_u64();
}
// finds magic values to create bijections between blockers and attacks for bishops and roks
// if found, attack index can be found by doing index = ((blocker & relevant_mask) * magic) >> shift
// it is the caller responsability to provide a large enough attack table
template <piece_t pc>
void init_magics(magic_t magics[NB_SQUARES], bitboard_t out_attacks[])
{
    static_assert(pc == ROOK | pc == BISHOP);
    srand(time(NULL));
    size_t offset = 0;
    for (square_t sq = A1; sq < NB_SQUARES; ++sq)
    {
        bitboard_t mask = relevancy_mask_bb<pc>(sq);
        std::cout << bb_format_string(mask);
        int        nb_ones      = popcount(mask);
        int        combinations = 1 << nb_ones;
        int        shift        = 64 - nb_ones;
        bitboard_t magic        = 0;
        assert(combinations <= 8192);
        bitboard_t blockers[8192];
        bitboard_t attacks[8192];
        for (int c = 0; c < combinations; c++)
        {
            blockers[c] = bb_from_mask(mask, nb_ones, c);
            attacks[c]  = sliding_attack_bb<pc>(sq, blockers[c]);
        }
        // maps the indexes already taken, to avoid memset 0 we mark a used index by the value of
        // the try it was set in
        uint64_t tries_map[8192];
        memset(tries_map, 0, sizeof(tries_map));
        for (int tries = 1;; tries++)
        {
            // if generator is changed make sure it has a high entropy
            magic     = random_magic();
            bool fail = false;
            for (int c = 0; c < combinations; c++)
            {
                size_t index = (blockers[c] * magic) >> shift;
                assert(index < combinations);
                bitboard_t cur = out_attacks[offset + index];
                if ((tries_map[index] == tries) && (cur != attacks[c]))
                {
                    fail = true;
                    break;
                }
                else
                {
                    out_attacks[offset + index] = attacks[c];
                    tries_map[index]            = tries;
                }
            }
            if (!fail)
            {
                break;
            }
        }
        magics[sq].offset = offset;
        magics[sq].magic  = magic;
        magics[sq].mask   = mask;
        magics[sq].shift  = shift;
        offset += combinations;
    }
}
void bb_init()
{
    init_bb_from_to();
    init_bb_base_attacks();
    init_magics<BISHOP>(bishop_magic, bishop_attacks);
    init_magics<ROOK>(rook_magics, rook_attacks);
}

std::string bb_format_string(bitboard_t bb)
{
    std::ostringstream out;
    out << "__________________\n";
    for (rank_t rank = RANK_1; rank <= RANK_8; ++rank)
    {
        out << "|";
        for (file_t file = FILE_A; file <= FILE_H; ++file)
        {
            out << ((bb & square_to_bb((square_t)((NB_RANKS - rank - 1) * 8 + file))) ? "X "
                                                                                      : ". ");
        }
        out << "|\n";
    }
    out << "__________________\n";
    return out.str();
}

int main(void)
{
    bb_init();
    bitboard_t bb = bb_from_to[A1][G7];
    int        df = file_of(A2) - file_of(B2);
    int        dr = rank_of(A2) - rank_of(B2);
    std::cout << dr << " " << df << std::endl;
    std::cout << bb_format_string(bb_from_to[B2][A2])
              << bb_format_string(sliding_attack_bb<SOUTH_WEST, EAST>(C3, 0, 2)) << std::endl;
    assert(bb_from_to[B2][A1] == sliding_attack_bb<SOUTH_WEST>(C3, 0, 2));
    std::cout << bb_format_string(bb);
    std::cout << bb_format_string(sliding_attack_bb<NORTH, SOUTH, NORTH_WEST>(C2));
    std::cout << bb_format_string(bb_piece_base_attack[KNIGHT - KNIGHT][C3]);
    std::cout << bb_format_string(bb_pawn_base_attack[BLACK][H5]);
    std::cout << bb_format_string(base_attack_bb<KNIGHT>(H5, WHITE));
    std::cout << bb_format_string(square_to_bb(D6));
    std::cout << bb_format_string(bishop_attacks[bishop_magic[C5].index(square_to_bb(D6))]);
    std::cout << bb_format_string(sliding_attack_bb<NORTH_EAST>(C5, square_to_bb(D6)));
    std::cout << bb_format_string(rook_attacks[rook_magics[C5].index(square_to_bb(E5))]);
    std::cout << bb_format_string(attacks_bb<QUEEN>(D5, square_to_bb(E6)));
    std::cout << bb_format_string(attacks_bb<ROOK>(H8, attacks_bb<QUEEN>(D5, 0)));
}
