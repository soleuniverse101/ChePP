//
// Created by paul on 7/29/25.
//

#ifndef EVAL_H
#define EVAL_H

#include "movegen.h"

using centipawn_type = int;

static constexpr enum_array<square_t, centipawn_type>  pawn_pst = {0,  0,  0,   0,  0,  0,   0,  0,  5,  10, 10, -20, -20, 10, 10, 5,
                                5,  -5, -10, 0,  0,  -10, -5, 5,  0,  0,  0,  20,  20,  0,  0,  0,
                                5,  5,  10,  25, 25, 10,  5,  5,  10, 10, 20, 30,  30,  20, 10, 10,
                                50, 50, 50,  50, 50, 50,  50, 50, 0,  0,  0,  0,   0,   0,  0,  0};

static constexpr enum_array<square_t, centipawn_type>  knight_pst = {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,   0,   -20, -40,
                                  -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,   15,  20,  20,  15,  5,   -30,
                                  -30, 0,   15,  20,  20,  15,  0,   -30, -30, 5,   10,  15,  15,  10,  5,   -30,
                                  -40, -20, 0,   5,   5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};


static constexpr enum_array<square_t, centipawn_type>  bishop_pst = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static constexpr enum_array<square_t, centipawn_type>  rook_pst = {
    0,   0,   5,  10,  10,   5,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0
};

static constexpr enum_array<square_t, centipawn_type>  queen_pst = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
      0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static constexpr enum_array<square_t, centipawn_type> king_pst = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  50,  10,   0,   0,  10,  50,  20
};

static constexpr enum_array<piece_type_t, enum_array<square_t, centipawn_type>> pst {
    pawn_pst, knight_pst, bishop_pst, rook_pst, queen_pst, king_pst
};

template <color_t us>
int evaluate_piece_tables(const position_t& pos) {
    int score = 0;

        for (auto pt = PAWN; pt <= KING; ++pt)
        {
            bitboard_t bb = pos.pieces_bb(us, pt);
            while (bb)
            {
                square_t sq{bb.pops_lsb()};
                score += pst.at(pt).at(us == WHITE ? sq : sq.flipped_horizontally());
            }

        }
    return score;
}



inline int evaluate_pawn_structure(const position_t& pos, color_t c)
{
    int        score = 0;
    bitboard_t pawns = pos.pieces_bb(c, PAWN);

    for (file_t file = FILE_A; file <= FILE_H; file = ++file)
    {
        const bitboard_t file_bb   = bb::file(file);
        const bitboard_t col_pawns = pawns & file_bb;
        const int        count     = col_pawns.popcount();

        // doubled
        if (count > 1)
        {
            score -= 20 * (count - 1);
        }

        // isolated
        const bool has_left{ pawns & bb::shift<EAST>(file_bb)};
        if (const bool has_right{pawns & bb::shift<WEST>(file_bb)}; count > 0 && !(has_left || has_right))
        {
            score -= 15;
        }
    }

    // passed
    const bitboard_t enemy_pawns = pos.pieces_bb(~c, PAWN);
    while (pawns)
    {
        const auto sq = static_cast<square_t>(pawns.pops_lsb());

        const file_t file = sq.file();
        bitboard_t   mask = bb::file(file);
        mask |= bb::shift<EAST>(mask);
        mask |= bb::shift<WEST>(mask);

        bitboard_t forward  = (c == WHITE) ? bb::ray<NORTH>(sq) : bb::ray<SOUTH>(sq);
        bitboard_t blockers = forward & mask & enemy_pawns;

        if (blockers == bb::empty())
        {
            score += 20 * (32 - pos.occupancy().popcount()) / 10;
        }
    }

    return score;
}

template <color_t c>
int evaluate_mobility(position_t& pos)
{
    move_list_t moves;
    gen_legal<c>(pos, moves);
    return bit::popcount(moves.size()) * 2;
}

template <color_t us>
inline int evaluate_king_safety(const position_t& pos)
{
    int        score  = 0;
    const auto ksq    = square_t{pos.pieces_bb(us, KING).get_lsb()};
    const auto ksq_bb = bb::square(ksq);

    const auto [file, rank] = ksq.coordinates();

    bitboard_t            shield{};
    constexpr auto up{relative_dir<us, NORTH>};
    constexpr auto right{relative_dir<us, EAST>};
    constexpr auto up_right{up + right};
    constexpr auto up_left{up - right};

    constexpr auto first_rank = relative_rank<us, RANK_1>;

    if (rank == first_rank)
    {
        shield = (ksq_bb | bb::shift<up_right>(ksq_bb) | bb::shift<up_left>(ksq_bb));
    }
    else
    {
        return 0;
    }

    const bitboard_t own_pawns = pos.pieces_bb(us, PAWN);
    const int        missing   = (shield & ~own_pawns).popcount();
    score -= 15 * missing;

    return score;
}

template <color_t us>
int evaluate_open_files(const position_t& pos) {
    int score = 0;
    bitboard_t rooks = pos.pieces_bb(us, ROOK);
    const bitboard_t pawns = pos.pieces_bb(PAWN);

    while (rooks) {
        square_t sq{rooks.pops_lsb()};
        const file_t file = sq.file();

        if (const bitboard_t file_bb = bb::file(file); (pawns & file_bb) == bb::empty()) {
            score += 25;
        } else if ((pos.pieces_bb(us, PAWN) & file_bb) == bb::empty()) {
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
        const int mult = (c == us) ? 1 : -1;

        for (piece_type_t pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN}) {
            score += mult * pos.pieces_bb(c, pt).popcount() * pt.value();
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

    score +=  evaluate_open_files<us>(pos);
    score -=    evaluate_open_files<other>(pos);


    return score;
}

#endif // EVAL_H
