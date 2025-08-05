//
// Created by paul on 8/3/25.
//

#ifndef NNUE_H
#define NNUE_H

#include "movegen.h"
#include "types.h"
#include "xsimd/xsimd.hpp"

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
        return feature_t(c * 64 + pt * 64 * 2 + sq * 64 * 2 * 6);
    }

    [[nodiscard]] uint16_t view(const square_t ksq) const { return ksq + m_idx; }

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

template <typename arch>
struct nnue_t
{
    nnue_t() = default;


    static constexpr size_t s_accumulator_size = 256;

    using simd_reg                             = xsimd::batch<float, arch>;
    static constexpr std::size_t simd_reg_size = simd_reg::size;
    static_assert(s_accumulator_size % simd_reg_size == 0, "accumulator size must be a multiple of simd_reg_size.");

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
                    feature_t::make(static_cast<square_t>(move.to_sq() - down), PAWN, ~c);
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
                        feature_t::make(move.to_sq(), piece_piece_type(taken), ~c);
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
                        feature_t::make(move.to_sq(), piece_piece_type(taken), ~c);
                }
                break;
            }
            default:;
        }
    }

    void update_accumulator(const position_t& pos)
    {

        auto& accumulator = m_accumulator_stack[m_accumulator_idx];
        constexpr std::size_t simd_size = simd_reg_size;
        constexpr std::size_t acc_size = s_accumulator_size;

        // Process removed features
        for (size_t i = 0; i < m_remove_features_idx; ++i)
        {
            for (const auto side : {WHITE, BLACK})
            {
                const auto& f = m_remove_dirty_features[i];
                const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(side)));

                for (size_t j = 0; j < acc_size; j += simd_size)
                {
                    simd_reg acc = simd_reg::load_unaligned(&accumulator[side][j]);
                    simd_reg w   = simd_reg::load_unaligned(&weights[j]);
                    acc -= w;
                    acc.store_unaligned(&accumulator[side][j]);
                }
            }
        }

        // Process added features
        for (size_t i = 0; i < m_add_features_idx; ++i)
        {
            for (const auto side : {WHITE, BLACK})
            {
                const auto& f = m_add_dirty_features[i];
                const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(side)));

                for (size_t j = 0; j < acc_size; j += simd_size)
                {
                    simd_reg acc = simd_reg::load_unaligned(&accumulator[side][j]);
                    simd_reg w   = simd_reg::load_unaligned(&weights[j]);
                    acc += w;
                    acc.store_unaligned(&accumulator[side][j]);
                }
            }
        }

        m_remove_features_idx = 0;
        m_add_features_idx = 0;
    }
};


#endif // NNUE_H
