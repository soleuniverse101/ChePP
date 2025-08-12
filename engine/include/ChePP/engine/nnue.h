//
// Created by paul on 8/3/25.
//

#ifndef NNUE_H
#define NNUE_H

#include "movegen.h"
#include "string.h"
#include "types.h"

#include <any>
#include <cmath>
#include <fstream>
#include <hwy/highway.h>
#include <hwy/highway_export.h>
#include <array>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "network_net.h"



HWY_BEFORE_NAMESPACE();
namespace hn = hwy::HWY_NAMESPACE;
HWY_AFTER_NAMESPACE();

struct WeightSource
{
    virtual ~WeightSource()                 = default;
    virtual void read_bytes(void* dst, long n) = 0;
};

struct MmapSource final : WeightSource
{
    const uint8_t* ptr;
    size_t         offset = 0;

    explicit MmapSource(const uint8_t* p) : ptr(p) {}
    void read_bytes(void* dst, const long n) override
    {
        std::memcpy(dst, ptr + offset, n);
        offset += n;
    }
};

struct FileSource final : WeightSource
{
    std::ifstream f;
    explicit FileSource(const std::string& path) { f.open(path, std::ios::binary); }
    void read_bytes(void* dst, const long n) override
    {
        f.read(static_cast<char*>(dst), n);
        if (f.eof())
        {
            throw std::runtime_error("File read error");
        }
    }
};

struct Deserializer
{
    WeightSource& src;
    bool             swap_endian;

    explicit Deserializer(WeightSource& s, const bool swap = false) : src(s), swap_endian(swap) {}

    template <typename DiskT, typename MemT>
    void read_array(MemT* dst, size_t count) const
    {
        if constexpr (std::is_same_v<DiskT, MemT> && !needs_swap<DiskT>())
        {
            src.read_bytes(dst, sizeof(DiskT) * count);
        }
        else
        {
            std::vector<DiskT> temp(count);
            src.read_bytes(temp.data(), sizeof(DiskT) * count);
            for (size_t i = 0; i < count; ++i)
            {
                DiskT val = temp[i];
                if constexpr (!std::is_same_v<DiskT, uint8_t> && !std::is_same_v<DiskT, int8_t>)
                {
                    if (swap_endian)
                        val = byte_swap(val);
                }
                dst[i] = static_cast<MemT>(val);
            }
        }
    }

  private:
    template <typename T>
    static constexpr bool needs_swap()
    {
        return sizeof(T) > 1;
    }

    template <typename T>
    static T byte_swap(T v)
    {
        if constexpr (sizeof(T) == 2)
        {
            return std::byteswap(reinterpret_cast<std::uint16_t&>(v));
        }
        else if constexpr (sizeof(T) == 4)
        {
            return std::byteswap(reinterpret_cast<std::uint32_t&>(v));

        }
        else if constexpr (sizeof(T) == 8)
        {
            return std::byteswap(reinterpret_cast<std::uint64_t&>(v));
        }
        else
        {
            return v;
        }
    }
};

struct NnueParser
{
    explicit NnueParser(const Deserializer& deserializer) : m_deserializer(deserializer) {}
    Deserializer m_deserializer;

    virtual ~NnueParser() = default;

    virtual void parse_header() const  = 0;
    virtual void parse_layer_header() const = 0;

    template <typename Network>
    void load_network(Network& net) const {
        parse_header();
        auto layers = net.layers();
        std::apply([&](auto&... layer_ptr) {
            (load_one_layer(*layer_ptr), ...);
        }, layers);
    }

private:
    template <typename Layer>
    void load_one_layer(Layer& layer) const {
        parse_layer_header();
        layer.init_impl(m_deserializer);
    }
};

struct DotNetParser final : NnueParser {

    explicit DotNetParser(const Deserializer& deserializer) : NnueParser(deserializer) {}

    void parse_header() const override
    {
    }

    void parse_layer_header() const override
    {
    }
};



template <typename T, std::size_t N>
struct DenseBuffer {
    using value_type = T;
    static constexpr std::size_t size_v = N;

    std::array<T, N> data{};

    constexpr std::size_t size() const noexcept { return N; }
    T*       data_ptr() noexcept { return data.data(); }
    const T* data_ptr() const noexcept { return data.data(); }

    T&       operator[](std::size_t i) noexcept { return data[i]; }
    const T& operator[](std::size_t i) const noexcept { return data[i]; }
};

struct AccumulatorInput
{
    static constexpr std::size_t size_v = 1;

    void clear()
    {
        m_added.clear();
        m_removed.clear();
    }

    template <bool IsAdd>
    void add(const std::size_t idx)
    {
        if constexpr (IsAdd)
        {
            m_added.push_back(idx);
        }
        else
        {
            m_removed.push_back(idx);
        }
    }

    [[nodiscard]] auto& added() const
    {
        return m_added;
    }

    [[nodiscard]] auto& removed() const
    {
        return m_removed;
    }

    [[nodiscard]] bool refresh() const
    {
        return m_refresh;
    }

private:
    std::vector<std::size_t> m_added{};
    std::vector<std::size_t> m_removed{};
    bool m_refresh = false;
};




template <typename Derived,
          typename InputBufferT,
          typename OutputBufferT>
struct LayerBase {
    using input_buffer_type  = InputBufferT;
    using output_buffer_type = OutputBufferT;

    static constexpr std::size_t input_size_v  = InputBufferT::size_v;
    static constexpr std::size_t output_size_v = OutputBufferT::size_v;

    void forward(const InputBufferT& in, OutputBufferT& out) const {
        static_cast<const Derived*>(this)->forward_impl(in, out);
    }

    template <typename Deserializer>
    void init(const Deserializer& d) {
        static_cast<Derived*>(this)->init_impl(d);
    }
};


template <typename T, std::size_t Size, std::size_t Scale>
struct Quantizer : LayerBase<Quantizer<T, Size, Scale>, DenseBuffer<T, Size>, DenseBuffer<T, Size>> {
    using Input  = DenseBuffer<T, Size>;
    using Output = DenseBuffer<T, Size>;

    void forward_impl(const Input& in, Output& out) const {
        for (std::size_t i = 0; i < Size; ++i) out[i] = in[i] / static_cast<T>(Scale);
    }

    template <typename Deserializer>
    void init_impl(const Deserializer&) {}
};

template <typename InputT, typename OutputT, std::size_t InSz, std::size_t OutSz>
struct Dense : LayerBase<Dense<InputT, OutputT, InSz, OutSz>, DenseBuffer<InputT, InSz>, DenseBuffer<OutputT, OutSz>> {
    using Input  = DenseBuffer<InputT, InSz>;
    using Output = DenseBuffer<OutputT, OutSz>;

    std::array<std::array<InputT, InSz>, OutSz> weights{};
    std::array<OutputT, OutSz>                   bias{};

    void forward_impl(const Input& in, Output& out) const {
        for (std::size_t o = 0; o < OutSz; ++o) {
            OutputT sum = bias[o];
            for (std::size_t i = 0; i < InSz; ++i) sum += static_cast<OutputT>(in[i]) * weights[o][i];
            out[o] = sum;
        }
    }

    template <typename Deserializer>
    void init_impl(const Deserializer& d) {
        for (size_t o = 0; o < OutSz; ++o) d.template read_array<InputT, InputT>(weights[o].data(), InSz);
        d.template read_array<OutputT, OutputT>(bias.data(), OutSz);
    }
};

template <typename T, std::size_t Size>
struct ReLU : LayerBase<ReLU<T, Size>, DenseBuffer<T, Size>, DenseBuffer<T, Size>> {
    using Input  = DenseBuffer<T, Size>;
    using Output = DenseBuffer<T, Size>;

    void forward_impl(const Input& in, Output& out) const {
        for (std::size_t i = 0; i < Size; ++i) out[i] = std::max(T(0), in[i]);
    }

    template <typename Deserializer>
    void init_impl(const Deserializer&) {}
};

template <typename T, std::size_t Size, float ScaleFactor>
struct Sigmoid : LayerBase<Sigmoid<T, Size, ScaleFactor>, DenseBuffer<T, Size>, DenseBuffer<T, Size>> {
    using Input  = DenseBuffer<T, Size>;
    using Output = DenseBuffer<T, Size>;

    void forward_impl(const Input& in, Output& out) const {
        for (std::size_t i = 0; i < Size; ++i) out[i] = T(1) / (T(1) + std::exp(-in[i] * ScaleFactor));
    }

    template <typename Deserializer>
    void init_impl(const Deserializer&) {}
};


template <typename T>
struct is_accumulator : std::false_type {};

template <typename T>
inline constexpr bool is_accumulator_v = is_accumulator<T>::value;


template <typename OutT, std::size_t OutSz, std::size_t NFeatures>
struct Accumulator : LayerBase<Accumulator<OutT, OutSz, NFeatures>, std::pair<AccumulatorInput, AccumulatorInput>, DenseBuffer<OutT, 2 * OutSz>> {

    using OutputDense = DenseBuffer<OutT, 2 * OutSz>;
    alignas(64) std::array<OutT, OutSz * NFeatures> m_weights{};
    alignas(64) std::array<OutT, OutSz> m_bias{};

    void forward_impl(const std::pair<AccumulatorInput, AccumulatorInput>& in, OutputDense& out) const {
        const auto [white, black] = in;
        if (white.refresh())
        {
            std::memcpy(out.data_ptr(), m_bias.data(), OutSz * sizeof(OutT));
        }
        add_features(white.added(), out.data_ptr());
        remove_features(white.removed(), out.data_ptr());
        if (black.refresh())
        {
            std::memcpy(out.data_ptr() + OutSz, m_bias.data(), OutSz * sizeof(OutT));
        }
        add_features(black.added(), out.data_ptr() + OutSz);
        remove_features(black.removed(), out.data_ptr() + OutSz);
    }

    void add_features(const std::vector<std::size_t>& added, OutT* out) const {
        for (const auto idx : added) {
            assert(idx < NFeatures);
            for (std::size_t o = 0; o < OutSz; ++o) out[o] += m_weights[idx * OutSz + o];

        }
    }

    void remove_features(const std::vector<std::size_t>& removed, OutT* out) const {
        for (const auto idx : removed) {
            assert(idx < NFeatures);
            for (std::size_t o = 0; o < OutSz; ++o) out[o] -= m_weights[idx * OutSz + o];
        }
    }

    template <typename Deserializer>
    void init_impl(const Deserializer& d) {
        for (size_t f = 0; f < NFeatures; ++f) d.template read_array<OutT, OutT>(m_weights.data() + f * OutSz, OutSz);
        d.template read_array<OutT, OutT>(m_bias.data(), OutSz);

    }
};

template <typename OutT, std::size_t OutSz, std::size_t NFeatures>
struct is_accumulator<Accumulator<OutT, OutSz, NFeatures>> : std::true_type {};

template <typename... LayerTs>
struct Network {
    static_assert((std::is_class_v<LayerTs> && ...), "All LayerTs must be class types");

    std::tuple<std::unique_ptr<LayerTs>...> m_layers;


    Network(std::unique_ptr<LayerTs>... ls) : m_layers(std::move(ls)...) {}
    Network() : m_layers(std::make_unique<LayerTs>()...) {}

    auto layers() {
        return std::apply([](auto&... ptrs) {
            return std::forward_as_tuple(ptrs...);
        }, m_layers);
    }

    auto layers() const {
        return std::apply([](auto const&... ptrs) {
            return std::forward_as_tuple(ptrs...);
        }, m_layers);
    }


    template <typename FirstIn, typename LastOut>
    void forward(const FirstIn& in, LastOut& out) const {
        forward_impl<0>(in, out);
    }

private:
    template <std::size_t Index, typename InBuf, typename OutBuf>
    void forward_impl(const InBuf& in, OutBuf& out) const {
        using LayerT = std::tuple_element_t<Index, std::tuple<LayerTs...>>;
        static_assert(LayerT::input_size_v == InBuf::size_v, "Input size mismatch");

        auto& layer = *std::get<Index>(m_layers);
        typename LayerT::output_buffer_type tmp;
        layer.forward(in, tmp);

        if constexpr (Index + 1 == sizeof...(LayerTs)) {
            static_assert(LayerT::output_size_v == OutBuf::size_v, "Output size mismatch");
            out = tmp;
        } else {
            forward_impl<Index + 1>(tmp, out);
        }
    }
};


template <typename AccumulatorT, typename... LayerTs>
struct NNUE : Network<LayerTs...> {
    static_assert(is_accumulator_v<AccumulatorT>, "First template arg must be an accumulator");

    using Base = Network<LayerTs...>;
    using AccBuf = AccumulatorT::output_buffer_type;

    explicit NNUE(std::unique_ptr<AccumulatorT> acc, std::unique_ptr<LayerTs>... ls)
        : Base(std::move(ls)...), m_accumulator(std::move(acc))
    {
        push_state();
    }
    NNUE() : NNUE(std::make_unique<AccumulatorT>(),std::make_unique<LayerTs>()...) {}

    void push_state() {
        m_state_stack.push_back(m_state);
    }

    void pop_accumulator() {
        if (m_state_stack.empty())
            throw std::runtime_error("Accumulator stack underflow");
        m_state = m_state_stack.back();
        m_state_stack.pop_back();
    }

    auto layers() {
        return std::tuple_cat(std::forward_as_tuple(m_accumulator), Base::layers());
    }

    auto layers() const {
        return std::tuple_cat(std::forward_as_tuple(m_accumulator), Base::layers());
    }

    template <typename LastOut>
    void forward(const std::pair<AccumulatorInput, AccumulatorInput>& in, LastOut& out) const {
        m_accumulator->forward(in, m_state);
        Base::forward(m_state, out);
    }
private:
    std::unique_ptr<AccumulatorT> m_accumulator;
    std::vector<AccBuf> m_state_stack{};
    mutable AccumulatorT::output_buffer_type m_state{};

};



/**





template <size_t sz>
struct accumulator_t
{
    std::array<int16_t, sz>& operator[](const color_t c)
    {
        assert(c == WHITE || c == BLACK);
        return c == WHITE ? m_white : m_black;
    }

    alignas(64) std::array<int16_t, sz> m_white;
    alignas(64) std::array<int16_t, sz> m_black;
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
    alignas(64) std::array<std::array<T, out_sz>, in_sz> m_weights;
    alignas(64) std::array<int16_t, out_sz> m_biases;
};

struct nnue_t
{
    nnue_t() = default;

    using D   = hn::ScalableTag<int16_t>;
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
                auto [k_from, k_to]                                 = move.castling_type().king_move();
                auto [r_from, r_to]                                 = move.castling_type().rook_move();
                m_add_dirty_features.at(m_add_features_idx++)       = feature_t::make(r_to, ROOK, c);
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(r_from, ROOK, c);
                m_add_dirty_features.at(m_add_features_idx++)       = feature_t::make(k_to, KING, c);
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(k_from, KING, c);
                break;
            }
            case EN_PASSANT:
            {
                constexpr direction_t down                          = c == WHITE ? SOUTH : NORTH;
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(move.from_sq(), PAWN, c);
                m_add_dirty_features.at(m_add_features_idx++)       = feature_t::make(move.to_sq(), PAWN, c);
                m_remove_dirty_features.at(m_remove_features_idx++) = feature_t::make(move.to_sq() - down, PAWN, ~c);
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

    void update_accumulator(const position_t& pos, const color_t c)
    {
        constexpr D      d;
        constexpr size_t lanes = hn::Lanes(d);

        auto& accumulator = m_accumulator_stack[m_accumulator_idx];

        // Removed features
        for (size_t i = 0; i < m_remove_features_idx; ++i)
        {
            const auto  f       = m_remove_dirty_features[i];
            const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(c)));

            for (size_t j = 0; j < s_accumulator_size; j += lanes)
            {
                const auto acc = hn::LoadU(d, &accumulator[c][j]);
                const auto w   = hn::LoadU(d, &weights[j]);
                hn::StoreU(hn::Sub(acc, w), d, &accumulator[c][j]);
            }
        }

        // Added features
        for (size_t i = 0; i < m_add_features_idx; ++i)
        {
            const auto  f       = m_add_dirty_features[i];
            const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(c)));

            for (size_t j = 0; j < s_accumulator_size; j += lanes)
            {
                const auto acc = hn::Load(d, &accumulator[c][j]);
                const auto w   = hn::Load(d, &weights[j]);
                hn::Store(hn::Add(acc, w), d, &accumulator[c][j]);
            }
        }
        m_add_features_idx    = 0;
        m_remove_features_idx = 0;
    }

    void refresh_accumulator(const position_t& pos, const color_t c)
    {
        using D = hn::ScalableTag<int16_t>;
        constexpr D      d;
        constexpr size_t lanes = hn::Lanes(d);

        auto& accumulator = m_accumulator_stack[m_accumulator_idx];

        memcpy(&accumulator[c], &m_accumulator_layer.m_biases, sizeof(accumulator));

        for (auto sq = A1; sq < H8; ++sq)
        {
            const auto pc = pos.piece_type_at(sq);
            if (pc == NO_PIECE_TYPE)
            {
                continue;
            }
            const auto  f       = feature_t::make(sq, pc, c);
            const auto& weights = m_accumulator_layer.m_weights.at(f.view(pos.ksq(c)));

            for (size_t j = 0; j < s_accumulator_size; j += lanes)
            {
                const auto acc = hn::Load(d, &accumulator[c][j]);
                const auto w   = hn::Load(d, &weights[j]);
                hn::Store(hn::Add(acc, w), d, &accumulator[c][j]);
            }
        }
    }
};

**/

#endif // NNUE_H
