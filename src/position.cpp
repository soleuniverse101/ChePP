#include "position.h"
#include "prng.h"
#include "types.h"

static constexpr piece_type_t Pieces[] = {W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
                                   B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING};

// at initiliaization, to get starting keys for the Zobrist hash
void position_t::init()  {
    PRNG rng(1070372);

    // init the keys in the Zobrist namespace
    for (piece_type_t pc : Pieces)
        for (square_t s = SQ_A1; s <= SQ_H8; ++s)
            Zobrist::psq[pc][s] = rng.rand<hash_t>();


    for (file_t f = FILE_A; f <= FILE_H; ++f)
        Zobrist::enpassant[f] = rng.rand<hash_t>();

    for (int cr = NO_CASTLING; cr <= ANY_CASTLING; ++cr)
        Zobrist::castling[cr] = rng.rand<hash_t>();

    Zobrist::side    = rng.rand<Key>();
    Zobrist::noPawns = rng.rand<Key>();

    // some other hash were in the source code, but unclear use for now so to add in later
}