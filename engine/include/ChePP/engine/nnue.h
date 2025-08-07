//
// Created by paul on 8/3/25.
//

#ifndef NNUE_H
#define NNUE_H

#include "types.h"
#include "movegen.h"
#include <hwy/highway.h>
#include <hwy/highway_export.h>
#include "string.h"


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
    feature_t() : m_idx(0) {}
    explicit feature_t(const int idx) : m_idx(idx) {}

    static feature_t make(const square_t sq, const piece_type_t pt, const color_t c)
    {
        return feature_t(value_of(c) * 64 + value_of(pt) * 64 * 2 + value_of(sq) * 64 * 2 * 6);
    }

    [[nodiscard]] uint16_t view(const square_t ksq) const { return value_of(ksq) + m_idx; }

    static constexpr size_t n_features = 64 * 64 * 6 * 2;

  private:
    uint16_t m_idx;
};

template <typename T, size_t in_sz, size_t out_sz>
struct layer_t
{
    std::array<std::array<T, out_sz>, in_sz> m_weights;
    std::array<int16_t, out_sz>              m_biases;
};

HWY_BEFORE_NAMESPACE();
namespace hn = hwy::HWY_NAMESPACE;
HWY_AFTER_NAMESPACE();


struct nnue_t
{
    nnue_t() = default;

    using D = hn::ScalableTag<int16_t>;
    using Vec = hn::Vec<D>;

    static constexpr size_t s_accumulator_size = 256;

    layer_t<int16_t, feature_t::n_features, s_accumulator_size> m_accumulator_layer{};
    std::array<accumulator_t<s_accumulator_size>, MAX_PLY>      m_accumulator_stack{};
    size_t                                                      m_accumulator_idx{};

    static constexpr size_t                                   s_hidden_layer_size = 256;
    layer_t<int16_t, s_accumulator_size, s_hidden_layer_size> m_hidden_layer{};

    std::array<feature_t, MAX_PLY * 4> m_add_dirty_features{};
    size_t                             m_add_features_idx{};
    std::array<feature_t, MAX_PLY * 4> m_remove_dirty_features{};
    size_t                             m_remove_features_idx{};

    void push_accumulator()
    {
        m_accumulator_stack[m_accumulator_idx + 1] = m_accumulator_stack[m_accumulator_idx];
        m_accumulator_idx++;
    }

    void pop_accumulator() { m_accumulator_idx--; }

    // assumes move has been played on the board
    template <color_t c>
    void add_dirty_move(position_t& pos, const move_t move)
    {
        switch (move.type_of())
        {
            case CASTLING:
            {
                auto [k_from, k_to]                           = castling_rights_t::king_move(move.castling_type());
                auto [r_from, r_to]                           = castling_rights_t::rook_move(move.castling_type());
                m_add_dirty_features.at(m_add_features_idx++) = feature_t::make(r_to, ROOK, c);
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(r_from, ROOK, c);
                m_add_dirty_features.at(m_add_features_idx++)      = feature_t::make(k_to, KING, c);
                m_remove_dirty_features.at(m_remove_features_idx++)= feature_t::make(k_from, KING, c);
                break;
            }
            case EN_PASSANT:
            {
                constexpr direction_t down                          = c == WHITE ? SOUTH : NORTH;
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(move.from_sq(), PAWN, c);
                m_add_dirty_features.at(m_add_features_idx++)      = feature_t::make(move.to_sq(), PAWN, c);
                m_remove_dirty_features.at(m_remove_features_idx++) =
                    feature_t::make(move.to_sq()- down, PAWN, ~c);
                break;
            }
            case NORMAL:
            {
                m_add_dirty_features.at(m_add_features_idx++) =
                    feature_t::make(move.to_sq(), pos.piece_type_at(move.to_sq()), c);
                m_remove_dirty_features.at(m_remove_features_idx++) =
                    feature_t::make(move.from_sq(), pos.piece_type_at(move.to_sq()), c);
                if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                {
                    m_remove_dirty_features.at(m_remove_features_idx++) =
                        feature_t::make(move.to_sq(), taken.type(), ~c);
                }
                break;
            }
            case PROMOTION:
            {
                m_add_dirty_features.at(m_add_features_idx++) = feature_t::make(move.to_sq(), move.promotion_type(), c);
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(move.from_sq(), PAWN, c);
                if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                {
                    m_remove_dirty_features.at(m_remove_features_idx++) =
                        feature_t::make(move.to_sq(), taken.type(), ~c);
                }
                break;
            }
            default:;
        }
    }

    void update_accumulator(const position_t& pos, const color_t c) {
        using D = hn::ScalableTag<int16_t>;
        constexpr D      d;
        constexpr size_t lanes = hn::Lanes(d);

        auto& accumulator = m_accumulator_stack[m_accumulator_idx];

        // Removed features
        for (size_t i = 0; i < m_remove_features_idx; ++i) {
                const auto f = m_remove_dirty_features[i];
                const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(c)));

                for (size_t j = 0; j < s_accumulator_size; j += lanes) {
                    const auto acc = hn::LoadU(d, &accumulator[c][j]);
                    const auto w   = hn::LoadU(d, &weights[j]);
                    hn::StoreU(hn::Sub(acc, w), d, &accumulator[c][j]);
                }
        }

        // Added features
        for (size_t i = 0; i < m_add_features_idx; ++i) {
                const auto f = m_add_dirty_features[i];
                const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(c)));

                for (size_t j = 0; j < s_accumulator_size; j += lanes) {
                    const auto acc = hn::LoadU(d, &accumulator[c][j]);
                    const auto w   = hn::LoadU(d, &weights[j]);
                    hn::StoreU(hn::Add(acc, w), d, &accumulator[c][j]);
                }
            }
    }

    void refresh_accumulator(const position_t& pos, const color_t c)
    {
        using D = hn::ScalableTag<int16_t>;
        constexpr D      d;
        constexpr size_t lanes = hn::Lanes(d);

        auto& accumulator = m_accumulator_stack[m_accumulator_idx];

        memcpy(&accumulator[c], &m_accumulator_layer.m_biases, sizeof(accumulator));

        for (auto sq = A1; sq < H8; ++sq )
        {
            const auto pc = pos.piece_type_at(sq);
            if (pc == NO_PIECE_TYPE)
            {
                continue;
            }
            const auto f = feature_t::make(sq, pc, c);
            const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(c)));

            for (size_t j = 0; j < s_accumulator_size; j += lanes) {
                const auto acc = hn::LoadU(d, &accumulator[c][j]);
                const auto w   = hn::LoadU(d, &weights[j]);
                hn::StoreU(hn::Add(acc, w), d, &accumulator[c][j]);
            }
        }
    }

};


#endif // NNUE_H
