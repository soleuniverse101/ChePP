//
// Created by paul on 8/3/25.
//

#ifndef NNUE_H
#define NNUE_H

#include "movegen.h"
#include "types.h"

template <size_t sz>
struct accumulator_t
{

    std::array<int16_t, sz>& operator[](const color_t c)
    {
        assert(c == WHITE || c == BLACK);
        return c == WHITE ? m_white : m_black;
    }

    std::array<int16_t, sz> m_white;
    std::array<int16_t, sz> m_black;
};

// (ksq, piece type, color, square)
//
class feature_t
{
public:
    feature_t(const int idx , const bool add) : m_idx(idx), m_add(add)
    {
    }

    static feature_t make(const square_t sq, const piece_type_t pt, const color_t c, const bool add)
    {
        return feature_t(c * 64 + pt * 64 * 2 + sq * 64 * 2 * 6, add);
    }

    uint16_t view(const square_t ksq) const {
        return ksq + m_idx;
    }

private:
    const uint16_t m_idx;
    const bool     m_add;

};


    struct nnue_t
    {
        std::array<accumulator_t<256>, 256> m_accumulator_stack;
        size_t                              m_accumulator_idx;

        std::array<feature_t, 256> m_dirty_features;
        size_t                     m_features_idx;

        void push_accumulator()
        {
            m_accumulator_stack[m_accumulator_idx + 1] = m_accumulator_stack[m_accumulator_idx];
            m_accumulator_idx++;
        }

        void pop_accumulator()
        {
            m_accumulator_idx--;
        }

        // assumes move has been played on the board
        template <color_t c>
        void add_dirty_move(position_t& pos, const move_t move)
        {
            switch (move.type_of())
            {
                case CASTLING:
                {
                    auto [k_from, k_to] = castling_rights_t::king_move(move.castling_type());
                    auto [r_from, r_to] = castling_rights_t::rook_move(move.castling_type());
                    m_dirty_features.at[m_features_idx++] = feature_t::make(r_to, ROOK, c, true);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(r_from, ROOK, c, false);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(k_to, KING, c, true);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(k_from, KING, c, false);
                    break;
                }
                case EN_PASSANT:
                {
                    constexpr direction_t down = c == WHITE ? SOUTH : NORTH;
                    m_dirty_features.at[m_features_idx++] = feature_t::make(move.from_sq(), PAWN, c, false);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(move.to_sq(), PAWN, c, true);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(static_cast<square_t>(move.to_sq() - down), PAWN, ~c, false);
                    break;
                }
                case NORMAL:
                {
                    m_dirty_features.at[m_features_idx++] = feature_t::make(move.to_sq(), pos.piece_type_at(move.to_sq()), c, true);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(move.from_sq(), pos.piece_type_at(move.to_sq()), c, false);
                    if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                    {
                        m_dirty_features.at[m_features_idx++] = feature_t::make(move.to_sq(), piece_piece_type(taken), ~c, false);
                    }
                    break;
                }
                case PROMOTION:
                {
                    m_dirty_features.at[m_features_idx++] = feature_t::make(move.to_sq(), move.promotion_type(), c, true);
                    m_dirty_features.at[m_features_idx++] = feature_t::make(move.from_sq(), PAWN, c, false);
                    if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                    {
                        m_dirty_features.at[m_features_idx++] = feature_t::make(move.to_sq(), piece_piece_type(taken), ~c, false);
                    }
                    break;
                }
                default:;
            }
        }

        void update_accumulator()
        {

        }
    };

#endif // NNUE_H
