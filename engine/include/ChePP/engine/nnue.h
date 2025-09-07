#ifndef SIMPLE_NNUE_H
#define SIMPLE_NNUE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "network_net.h"
#include "position.h"

template <typename T, size_t MaxSize>
class ArrayStack
{
    std::array<T, MaxSize> data{};
    size_t                 topIndex = 0;

  public:
    bool empty() const { return topIndex == 0; }
    bool full() const { return topIndex == MaxSize; }

    bool push_back(const T& value)
    {
        if (full())
            return false;
        data[topIndex++] = value;
        return true;
    }

    bool pop()
    {
        if (empty())
            return false;
        --topIndex;
        return true;
    }

    T& top()
    {
        if (empty())
            throw std::underflow_error("Stack empty");
        return data[topIndex - 1];
    }

    const T& top() const
    {
        if (empty())
            throw std::underflow_error("Stack empty");
        return data[topIndex - 1];
    }

    size_t size() const { return topIndex; }

    auto begin() { return data.begin(); }
    auto end() { return data.begin() + topIndex; }

    auto begin() const { return data.begin(); }
    auto end() const { return data.begin() + topIndex; }
};

struct FeatureTransformer
{
    static constexpr auto MaxChanges = 32;
    using FeatureT                   = uint16_t;
    using RetT                       = ArrayStack<FeatureT, MaxChanges>;

    static constexpr size_t n_features_v = 16 * 12 * 64;

    static bool needs_refresh(const Position& cur, const Position& prev, const Color view)
    {
        return king_square_index(cur.ksq(view)) != king_square_index(prev.ksq(view));
    }

    static std::pair<RetT, RetT> get_features(const Position& cur, const Position& prev, const Color view, const bool refresh)

    {
        RetT add_v;
        RetT rem_v;

        if (refresh)
        {
            cur.occupancy().for_each_square([&](const Square& sq)
                                            { add_v.push_back(get_index(view, cur.ksq(view), sq, cur.piece_at(sq))); });
        }
        else
        {
            auto add = [&](const Square sq, const Piece pc)
            { add_v.push_back(get_index(view, cur.ksq(view), sq, pc)); };
            auto rem = [&](const Square sq, const Piece pc)
            { rem_v.push_back(get_index(view, cur.ksq(view), sq, pc)); };

            const EnumArray<Color, Bitboard> color_diff = {
                prev.occupancy(WHITE) ^ cur.occupancy(WHITE),
                prev.occupancy(BLACK) ^ cur.occupancy(BLACK),
            };

            for (const auto c : {WHITE, BLACK})
            {
                color_diff.at(c).for_each_square(
                    [&](const Square& sq)
                    {
                        if (prev.occupancy(c).is_set(sq))
                        {
                            rem(sq, prev.piece_at(sq));
                        }
                        else
                        {
                            add(sq, cur.piece_at(sq));
                        }
                    });
            }
        }
        return {add_v, rem_v};
    }

  private:
    static FeatureT get_index(const Color view, const Square ksq, const Square sq, const Piece piece)
    {
        constexpr int PIECE_TYPE_FACTOR  = 64;
        constexpr int PIECE_COLOR_FACTOR = PIECE_TYPE_FACTOR * 6;
        constexpr int KING_SQUARE_FACTOR = PIECE_COLOR_FACTOR * 2;

        const Square relative_king_sq  = (view == WHITE ? ksq : ksq.flipped_horizontally());
        Square       relative_piece_sq = (view == WHITE ? sq : sq.flipped_horizontally());

        const FeatureT king_sq_idx = king_square_index(relative_king_sq);
        if (ksq.file().index() > 3)
            relative_piece_sq = relative_piece_sq.flipped_vertically();

        return relative_piece_sq.value() + piece.type().value() * PIECE_TYPE_FACTOR +
               (piece.color() == view ? PIECE_COLOR_FACTOR : 0) + king_sq_idx * KING_SQUARE_FACTOR;
    }

    static FeatureT king_square_index(const Square sq)
    {
        constexpr EnumArray<Square, FeatureT> indices = {
            0,  1,  2,  3,  3,  2,  1,  0,  4,  5,  6,  7,  7,  6,  5,  4,  8,  9,  10, 11, 11, 10,
            9,  8,  8,  9,  10, 11, 11, 10, 9,  8,  12, 12, 13, 13, 13, 13, 12, 12, 12, 12, 13, 13,
            13, 13, 12, 12, 14, 14, 15, 15, 15, 15, 14, 14, 14, 14, 15, 15, 15, 15, 14, 14};
        return indices[sq];
    }
};

#include <hwy/highway.h>


HWY_BEFORE_NAMESPACE();
using namespace hwy::HWY_NAMESPACE;

struct Accumulator {
    static constexpr auto OutSz = 512;
    using AccumulatorT = std::array<int16_t, OutSz>;

private:
    HWY_ALIGN AccumulatorT white_accumulator{};
    HWY_ALIGN AccumulatorT black_accumulator{};

public:
    Accumulator() = default;
    explicit Accumulator(const Position& pos) {
        const auto [wadd, wrem] = FeatureTransformer::get_features(pos, pos, WHITE, true);
        refresh_acc(WHITE, wadd);
        const auto [badd, brem] = FeatureTransformer::get_features(pos, pos, BLACK, true);
        refresh_acc(BLACK, badd);
    }

    explicit Accumulator(const Accumulator& acc_prev, const Position& pos_cur, const Position& pos_prev) {
        update(acc_prev, pos_cur, pos_prev, WHITE);
        update(acc_prev, pos_cur, pos_prev, BLACK);
    }

    template <size_t UNROLL = 4>
    int32_t evaluate(const Color view) {
        const auto our_acc_ptr   =
            static_cast<const int16_t*>(HWY_ASSUME_ALIGNED(view == WHITE ? white_accumulator.data() : black_accumulator.data(), 64));
        const auto their_acc_ptr =
            static_cast<const int16_t*>(HWY_ASSUME_ALIGNED(view == WHITE ? black_accumulator.data() : white_accumulator.data(), 64));

        int32_t out = g_hidden_biases[0];

        using D32 = ScalableTag<int32_t>;
        using D16 = ScalableTag<int16_t>;

        alignas(64) Vec<D32> acc = {};

        for (size_t i = 0; i < OutSz; i += UNROLL * Lanes(D16{})) {
            alignas(64) Vec<D16> v_our[UNROLL];
            alignas(64) Vec<D16> v_their[UNROLL];
            alignas(64) Vec<D16> w_our[UNROLL];
            alignas(64) Vec<D16> w_their[UNROLL];

            for (size_t u = 0; u < UNROLL; ++u) {
                if (i + u * Lanes(D16{}) < OutSz) {
                    v_our[u]   = Max(Load(D16{}, &our_acc_ptr[i + u * Lanes(D16{})]), Zero(D16{}));
                    v_their[u] = Max(Load(D16{}, &their_acc_ptr[i + u * Lanes(D16{})]), Zero(D16{}));

                    w_our[u]   = Load(D16{}, &g_hidden_weights[i + u * Lanes(D16{})]);
                    w_their[u] = Load(D16{}, &g_hidden_weights[i + OutSz + u * Lanes(D16{})]);

                    acc = Add(acc, WidenMulPairwiseAdd(D32{}, v_our[u], w_our[u]));
                    acc = Add(acc, WidenMulPairwiseAdd(D32{}, v_their[u], w_their[u]));
                }
            }
        }

        out += ReduceSum(D32{}, acc);

        out = (out / 128) / 32;
        return out;
    }


private:

    void update(const Accumulator& prev, const Position& pos_cur, const Position& pos_prev, const Color view) {
        const bool needs_refresh = FeatureTransformer::needs_refresh(pos_cur, pos_prev, view);
        const auto [add, rem]    = FeatureTransformer::get_features(pos_cur, pos_prev, view, needs_refresh);
        if (needs_refresh)
            refresh_acc(view, add);
        else
            update_acc(prev, view, add, rem);
    }

    template <size_t UNROLL = 8>
    void refresh_acc(const Color view, const FeatureTransformer::RetT& features) {
        auto& acc = (view == WHITE ? white_accumulator : black_accumulator);

        std::memcpy(acc.data(), g_ft_biases, OutSz * sizeof(int16_t));

        using D = ScalableTag<int16_t>;
        alignas(64) auto v_accumulators = std::array<decltype(Load(D{}, acc.data())), UNROLL>{};

        auto* HWY_RESTRICT acc_ptr =  static_cast<int16_t*>(HWY_ASSUME_ALIGNED(acc.data(), 64));

        for (size_t i = 0; i < OutSz; i += UNROLL * Lanes(D{})) {
            for (size_t u = 0; u < UNROLL; ++u) {
                if (i + u * Lanes(D{}) < OutSz)
                    v_accumulators[u] = Load(D{}, &acc_ptr[i + u * Lanes(D{})]);
            }

            for (const auto f : features) {
                for (size_t u = 0; u < UNROLL; ++u) {
                    if (i + u * Lanes(D{}) < OutSz) {
                        auto v_weights = Load(D{}, &g_ft_weights[f * OutSz + i + u * Lanes(D{})]);
                        v_accumulators[u] = Add(v_accumulators[u], v_weights);
                    }
                }
            }

            for (size_t u = 0; u < UNROLL; ++u) {
                if (i + u * Lanes(D{}) < OutSz)
                    Store(v_accumulators[u], D{}, &acc_ptr[i + u * Lanes(D{})]);
            }
        }
    }

    template <size_t UNROLL = 8>
    void update_acc(
                    const Accumulator& previous,
                    const Color view,
                    const FeatureTransformer::RetT& add,
                    const FeatureTransformer::RetT& sub) {
        auto& acc  = (view == WHITE ? white_accumulator : black_accumulator);
        auto& prev = (view == WHITE ? previous.white_accumulator : previous.black_accumulator);

        std::memcpy(acc.data(), prev.data(), OutSz * sizeof(int16_t));

        using D = ScalableTag<int16_t>;
        alignas(64) auto v_accumulators = std::array<decltype(Load(D{}, acc.data())), UNROLL>{};

        auto* HWY_RESTRICT acc_ptr = static_cast<int16_t*>(HWY_ASSUME_ALIGNED(acc.data(), 64));

        for (size_t i = 0; i < OutSz; i += UNROLL * Lanes(D{})) {
            for (size_t u = 0; u < UNROLL; ++u) {
                if (i + u * Lanes(D{}) < OutSz)
                    v_accumulators[u] = Load(D{}, &acc_ptr[i + u * Lanes(D{})]);
            }

            for (const auto f : add) {
                for (size_t u = 0; u < UNROLL; ++u) {
                    if (i + u * Lanes(D{}) < OutSz) {
                        auto v_weights = Load(D{}, &g_ft_weights[f * OutSz + i + u * Lanes(D{})]);
                        v_accumulators[u] = Add(v_accumulators[u], v_weights);
                    }
                }
            }

            for (const auto f : sub) {
                for (size_t u = 0; u < UNROLL; ++u) {
                    if (i + u * Lanes(D{}) < OutSz) {
                        auto v_weights = Load(D{}, &g_ft_weights[f * OutSz + i + u * Lanes(D{})]);
                        v_accumulators[u] = Sub(v_accumulators[u], v_weights);
                    }
                }
            }

            for (size_t u = 0; u < UNROLL; ++u) {
                if (i + u * Lanes(D{}) < OutSz)
                    Store(v_accumulators[u], D{}, &acc_ptr[i + u * Lanes(D{})]);
            }
        }
    }

};

HWY_AFTER_NAMESPACE();


#endif