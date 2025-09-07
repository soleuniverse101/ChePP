//
// Created by paul on 9/7/25.
//

#include "ChePP/engine/types.h"
#include "ChePP/engine/position.h"
#include <sstream>
#include <iostream>

std::string Move::to_algebraic(const Position& pos) const
{
    if (*this == none() || *this == null())
        return "--";

    const Piece  pc   = pos.piece_at(from_sq());
    const Color  us   = pc.color();
    const Color  them = ~us;
    const Square from = from_sq();
    const Square to   = to_sq();

    if (type_of() == CASTLING)
    {
        return castling_type().side() == KINGSIDE ? "O-O" : "O-O-O";
    }

    std::ostringstream oss;

    if (pc.type() != PAWN)
        oss << pc.type();

    Bitboard attacks = pos.attacking_sq(to, pos.occupancy()) & pos.occupancy(us, pc.type());
    attacks.unset(from);

    bool need_file = false, need_rank = false;
    attacks.for_each_square(
        [&](const Square sq)
        {
            need_file |= sq.rank() == from.rank();
            need_rank |= sq.file() == from.file();
        });

    if (need_file)
        oss << from.file();
    if (need_rank)
        oss << from.rank();

    if (pos.occupancy().is_set(to) || type_of() == EN_PASSANT)
    {
        if (pc.type() == PAWN && !need_file)
            oss << from.file();
        oss << "x";
    }

    oss << to;

    if (type_of() == PROMOTION)
    {
        oss << "=" << Piece{us, promotion_type()}.type();
    }

    if (pos.checkers(us))
    {
        const Bitboard moves = pseudo_attack<KING>(from, us);
        bool mate = true;

        (moves & (~pos.occupancy() | pos.occupancy(~us))) // unoccupied or capture
            .for_each_square(
                [&](const Square sq)
                {
                    mate &= !pos.is_legal(make<NORMAL>(from, sq));
                });

        oss << (mate ? "#" : "+");
    }

    return oss.str();
}
