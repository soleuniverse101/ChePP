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
struct feature_t
{
    feature_t(const square_t ksq, const square_t sq, const piece_type_t pt, const color_t c)
        : m_idx(static_cast<uint16_t>(sq) + ((pt * 2 + c) + ksq * 10) * 64)
    {
    }
    const uint16_t m_idx;
};

struct dirty_move_t
{

    dirty_move_t(const position_t& pos, const move_t move)
        : m_move(move)
    {
    }
          };

    struct nnue_t
    {
        std::array<accumulator_t<256>, 256> m_accumulator_stack;
        size_t                              m_accumulator_idx;

        std::array<feature_t, 256> m_dirty_features;
        size_t                     m_features_idx;

        void accumulator_update() {}

        void push(move_t mv) {}

        void pop() {}

        // assumes move has been played on the board
        template <color_t c>
        void update_accumulator(position_t& pos, const move_t move)
        {

            switch (move.type_of())
            {
                case CASTLING:
                {
                    auto [k_from, k_to] = castling_rights_t::king_move(move.castling_type());
                    auto [r_from, r_to] = castling_rights_t::rook_move(move.castling_type());
                }
                case NORMAL:
                {
                }
            }
        }
    };

#endif // NNUE_H
