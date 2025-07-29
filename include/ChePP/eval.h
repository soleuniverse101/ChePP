//
// Created by paul on 7/29/25.
//

#ifndef EVAL_H
#define EVAL_H

#include "movegen.h"

constexpr int pawn_pst[64] = {0,  0,  0,   0,  0,  0,   0,  0,  5,  10, 10, -20, -20, 10, 10, 5,
                                5,  -5, -10, 0,  0,  -10, -5, 5,  0,  0,  0,  20,  20,  0,  0,  0,
                                5,  5,  10,  25, 25, 10,  5,  5,  10, 10, 20, 30,  30,  20, 10, 10,
                                50, 50, 50,  50, 50, 50,  50, 50, 0,  0,  0,  0,   0,   0,  0,  0};

constexpr int knight_pst[64] = {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,   0,   -20, -40,
                                  -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,   15,  20,  20,  15,  5,   -30,
                                  -30, 0,   15,  20,  20,  15,  0,   -30, -30, 5,   10,  15,  15,  10,  5,   -30,
                                  -40, -20, 0,   5,   5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};


static constexpr int bishop_pst[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static constexpr int rook_pst[64] = {
    0,   0,   5,  10,  10,   5,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0
};

static constexpr int queen_pst[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
      0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static constexpr int king_pst[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20
};

template <color_t us>
int evaluate_piece_tables(const position_t& pos) {
    int score = 0;
        bitboard_t bb;

        bb = pos.pieces_bb(us, KNIGHT);
        while (bb) {
            int sq = pop_lsb(bb);
            score += knight_pst[us == WHITE ? sq : mirror_sq(sq)];
        }

        bb = pos.pieces_bb(us, PAWN);
        while (bb) {
            int sq = pop_lsb(bb);
            score += pawn_pst[us == WHITE ? sq : mirror_sq(sq)];
        }

        bb = pos.pieces_bb(us, BISHOP);
        while (bb) {
            int sq = pop_lsb(bb);
            score += bishop_pst[us == WHITE ? sq : mirror_sq(sq)];
        }

        bb = pos.pieces_bb(us, ROOK);
        while (bb)
        {
            int sq = pop_lsb(bb);
            score += rook_pst[us == WHITE ? sq : mirror_sq(sq)];
        }

        bb = pos.pieces_bb(us, QUEEN);
        while (bb) {
            int sq = pop_lsb(bb);
            score += queen_pst[us == WHITE ? sq : mirror_sq(sq)];
        }

        bb = pos.pieces_bb(us, KING);
        while (bb) {
            int sq = pop_lsb(bb);
            score += king_pst[us == WHITE ? sq : mirror_sq(sq)];
        }

    return score;
}



inline int evaluate_pawn_structure(const position_t& pos, color_t c)
{
    int        score = 0;
    bitboard_t pawns = pos.pieces_bb(c, PAWN);

    for (file_t file = FILE_A; file <= FILE_H; file = static_cast<file_t>(file + 1))
    {
        const bitboard_t file_bb   = bb::fl_mask(file);
        const bitboard_t col_pawns = pawns & file_bb;
        const int        count     = popcount(col_pawns);

        // doubled
        if (count > 1)
        {
            score -= 20 * (count - 1);
        }

        // isolated
        const bool has_left = pawns & bb::shift<EAST>(file_bb);
        if (const bool has_right = pawns & bb::shift<WEST>(file_bb); count > 0 && !(has_left || has_right))
        {
            score -= 15;
        }
    }

    // passed
    bitboard_t enemy_pawns = pos.pieces_bb(~c, PAWN);
    while (pawns)
    {
        const auto sq = static_cast<square_t>(pop_lsb(pawns));

        const file_t file = fl_of(sq);
        bitboard_t   mask = bb::fl_mask(file);
        mask |= bb::shift<EAST>(mask);
        mask |= bb::shift<WEST>(mask);

        bitboard_t forward  = (c == WHITE) ? bb::ray<NORTH>(sq) : bb::ray<SOUTH>(sq);
        bitboard_t blockers = forward & mask & enemy_pawns;

        if (blockers == 0)
        {
            score += 20;
        }
    }

    return score;
}

template <color_t c>
int evaluate_mobility(position_t& pos)
{
    move_list_t moves;
    gen_legal<c>(pos, moves);
    return popcount(moves.size()) * 2;
}

template <color_t us>
inline int evaluate_king_safety(const position_t& pos)
{
    int        score  = 0;
    const auto ksq    = static_cast<square_t>(get_lsb(pos.pieces_bb(us, KING)));
    const auto ksq_bb = bb::sq_mask(ksq);

    const rank_t rank = rk_of(ksq);
    const file_t file = fl_of(ksq);

    bitboard_t            shield;
    constexpr direction_t up       = us == WHITE ? NORTH : SOUTH;
    constexpr direction_t right    = us == WHITE ? EAST : WEST;
    constexpr direction_t up_right = static_cast<direction_t>(up + right);
    constexpr direction_t up_left  = static_cast<direction_t>(up - right);

    if (rank == 0)
    {

        shield = (ksq_bb | bb::shift<up_right>(ksq_bb) | bb::shift<up_left>(ksq_bb));
    }
    else
    {
        return 0;
    }

    bitboard_t own_pawns = pos.pieces_bb(us, PAWN);
    int        missing   = popcount(shield & ~own_pawns);
    score -= 15 * missing;

    return score;
}


int evaluate_open_files(const position_t& pos, color_t c) {
    int score = 0;
    bitboard_t rooks = pos.pieces_bb(c, ROOK);
    bitboard_t pawns = pos.pieces_bb(WHITE, PAWN) | pos.pieces_bb(BLACK, PAWN);

    while (rooks) {
        square_t sq = static_cast<square_t>(pop_lsb(rooks));
        file_t file = fl_of(sq);
        bitboard_t file_bb = bb::fl_mask(file);

        if ((pawns & file_bb) == 0) {
            score += 25;
        } else if ((pos.pieces_bb(c, PAWN) & file_bb) == 0) {
            score += 15;
        }
    }

    return score;
}

template <color_t us>
int evaluate_position( position_t& pos)
{
    constexpr color_t other = ~us;
    int score = 0;

    for (const color_t c : {WHITE, BLACK})
    {
        int mult = (c == us) ? 1 : -1;

        for (piece_type_t pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN}) {
            score += mult * popcount(pos.pieces_bb(c, pt)) * piece_value(pt);
        }
    }

    score += evaluate_pawn_structure(pos, us);
    score -= evaluate_pawn_structure(pos, other);

    score += evaluate_piece_tables<us>(pos);
    score -= evaluate_piece_tables<other>(pos);

    score +=  evaluate_mobility<us>(pos);
    score -= evaluate_mobility<other>(pos);

    score +=  evaluate_king_safety<us>(pos);
    score -= evaluate_king_safety<other>(pos);


    return score;
}

#endif // EVAL_H
