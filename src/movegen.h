#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include "bitboard.h"
#include "magics.h"
#include "position.h"

#include <memory>
#include <system_error>



 enum movegen_type_t : int8_t
{
    QUIET,
    CAPTURE,
    ESCAPE,
    PSEUDO_LEGAL,
    LEGAL
} ;


 enum move_type_t {
    NORMAL,
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14
} ;

// a move is encoded as an 16bit unsigned int
// 0-5 bit : to square (square 0 to 63)
// 6-11 bit : from square (square 0 to 63)
// 12-13 bit : promotion tpiece type (shifted by KNIGHT which is the lowest promotion to fit)
// 14-15: promotion (1), en passant (2), castling (3)
class move_t
{
  public:
    constexpr explicit move_t(const std::uint16_t d) : data(d) {}

    constexpr move_t(const square_t from, const square_t to) : data((from << 6) + to) {}

	// to build a move if you already know the type of move
    template<move_type_t T>
    static constexpr move_t make(const square_t from, const square_t to,
                                 const piece_type_t pt = KNIGHT) {
        return move_t(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
    }

	// for sanity check
    constexpr bool is_ok() const { return none().data != data && null().data != data; }

	// for these two moves from and to are the same
	static constexpr move_t null() { return move_t(65); }
    static constexpr move_t none() { return move_t(0); }

	constexpr bool operator==(const move_t& m) const { return data == m.data; }
    constexpr bool operator!=(const move_t& m) const { return data != m.data; }

	constexpr explicit operator bool() const { return data != 0; }

    constexpr std::uint16_t raw() const { return data; }

	constexpr square_t from_sq() const {
        assert(is_ok());
        return static_cast<square_t>((data >> 6) & 0x3F);
    }

    constexpr square_t to_sq() const {
        assert(is_ok());
        return static_cast<square_t>(data & 0x3F);
    }

	constexpr move_type_t type_of() const { return static_cast<move_type_t>(data & (3 << 14)); }

    constexpr piece_type_t promotion_type() const { return static_cast<piece_type_t>((data >> 12 & 3) + KNIGHT); }

  private:
    std::uint16_t data;
};

class move_list_t
{
  public:
    static constexpr size_t max_moves = 256;
    void                add(move_t m);
};


template <color_t c>
void gen_pawn_moves(position_t* pos, move_list_t* list, bitboard_t check_mask = bb::full)
{
    //check mask refers to king attackers u rays of king attackers
    constexpr direction_t up               = c == WHITE ? NORTH : SOUTH;
    constexpr direction_t right             = c == WHITE ? EAST : WEST;
    constexpr bitboard_t bb_promotion_rank = c == WHITE ? bb::rk_mask(RANK_7) : bb::rk_mask(RANK_2);
    constexpr bitboard_t bb_third_rank     = c == WHITE ? bb::rk_mask(RANK_3) : bb::rk_mask(RANK_5);
    const bitboard_t     available         = ~pos->color_occupancy(ANY);
    const bitboard_t     enemy             = pos->color_occupancy(~c);
    const bitboard_t     pawns             = pos->pieces_bb(c, PAWN);
    const bitboard_t     ep_bb =
        (pos->ep_square() == NO_SQUARE ? bb::empty : bb::sq_mask(pos->ep_square()));
    // straight
    {
        bitboard_t single_push = bb::shift<up>(pawns & ~bb_promotion_rank) & available;
        bitboard_t double_push = bb::shift<up>(single_push & bb_third_rank) & available & check_mask;
        single_push &= check_mask;
        while (single_push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(single_push));
            list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - up), sq));
        }
        while (double_push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(double_push));
            list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - 2 * up), sq));
        }
    }
    // promotion
    if (const bitboard_t promotions = pawns & bb_promotion_rank)
    {
        bitboard_t push = bb::shift<up>(promotions) & available & check_mask;
        while (push)
        {
            const auto sq = static_cast<square_t>(pop_lsb(push));
            list->add(move_t::make<PROMOTION>(static_cast<square_t>(sq - up), sq));
        }
        bitboard_t  take_right = bb::shift<up + right>(promotions) & enemy & check_mask;
        while (take_right)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_right));
            list->add(move_t::make<PROMOTION>(static_cast<square_t>(sq - (up  + right)), sq));
        }
        bitboard_t  take_left = bb::shift<up - right>(promotions) & enemy & check_mask;
        while (take_left)
        {
            const auto sq = static_cast<square_t>(pop_lsb(take_left));
            list->add(move_t::make<PROMOTION>(static_cast<square_t>(sq - (up  - right)), sq));
        }
    }
    //capture
    {
        bitboard_t capturable = enemy | ep_bb;
        bitboard_t  take_right = bb::shift<up + right>(pawns & ~bb_promotion_rank) & capturable & check_mask;
        while (take_right)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(take_right)); sq == pos->ep_square()) [[unlikely]]
            {
                list->add(move_t::make<EN_PASSANT>(static_cast<square_t>(sq - (up  + right)), sq));

            } else [[likely]]
            {
                list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - (up  + right)), sq));

            }
        }
        bitboard_t  take_left = bb::shift<up - right>(pawns & ~bb_promotion_rank) & capturable & check_mask;
        while (take_left)
        {
            if (const auto sq = static_cast<square_t>(pop_lsb(take_left)); sq == pos->ep_square()) [[unlikely]]
            {
                list->add(move_t::make<EN_PASSANT>(static_cast<square_t>(sq - (up  - right)), sq));

            } else [[likely]]
            {
                list->add(move_t::make<NORMAL>(static_cast<square_t>(sq - (up  - right)), sq));

            }
        }
    }
}


template <piece_type_t pc, color_t c>
void gen_pc_moves(position_t* pos, move_list_t* list, bitboard_t check_mask = bb::full)
{
    bitboard_t bb = pos->pieces_occupancy(c, pc);
    while (bb)
    {
        auto from = static_cast<square_t>(pop_lsb(bb));
        bitboard_t attacks = bb::attacks<pc>(from, pos->color_occupancy(ANY)) & ~pos->color_occupancy(c) & check_mask;
        while (attacks)
        {
            list->add(move_t::make<NORMAL>(from,static_cast<square_t>(pop_lsb(attacks))));
        }
    }
}


template <color_t c>
void gen_castling(position_t* pos, bitboard_t check_mask = bb::full)
{
    if (check_mask != bb::full) return;
    castling_rights_t crs = pos->crs() & castling_rights<c>();
    if (crs == 0) return;


}





#endif
