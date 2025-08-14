//
// Created by paul on 8/3/25.
//

#ifndef NNUE_H
#define NNUE_H

#include "string.h"
#include "types.h"

#include <any>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <hwy/highway.h>
#include <hwy/highway_export.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "network_net.h"

#include <functional>

HWY_BEFORE_NAMESPACE();
namespace hn = hwy::HWY_NAMESPACE;
HWY_AFTER_NAMESPACE();

struct FeatureContext
{
    explicit FeatureContext(const position_t& pos, const piece_t piece, const square_t piece_square)
        : m_piece(piece), m_piece_square(piece_square)
    {
    }

    piece_t  m_piece;
    square_t m_piece_square;
};

template <typename Derived, typename T, std::size_t NFeatures>
struct FeatureExtractorBase
{
    static constexpr std::size_t n_features = NFeatures;
    using FeatureT                          = T;

    template <color_t view>
    static FeatureT extract(const position_t& pos, const square_t square, const piece_t piece)
    {
        return Derived::template extract<view>(pos, square, piece);
    }
};

struct KoiFeatureExtractor : FeatureExtractorBase<KoiFeatureExtractor, uint32_t, 16 * 12 * 64>
{
    template <color_t view>
    static FeatureT extract(const position_t& pos, const square_t piece_square, const piece_t piece)
    {
        constexpr int PIECE_TYPE_FACTOR  = 64;
        constexpr int PIECE_COLOR_FACTOR = PIECE_TYPE_FACTOR * 6;
        constexpr int KING_SQUARE_FACTOR = PIECE_COLOR_FACTOR * 2;

        square_t relative_king_square{NO_SQUARE};
        square_t relative_piece_square{NO_SQUARE};

        square_t king_square = pos.ksq(view);

        if (view == WHITE)
        {
            relative_king_square  = king_square;
            relative_piece_square = piece_square;
        }
        else
        {
            relative_king_square  = king_square.flipped_horizontally();
            relative_piece_square = piece_square.flipped_horizontally();
        }

        const int king_square_idx = king_square_index(relative_king_square);
        if (index(king_square.file()) > 3)
        {
            relative_piece_square = relative_piece_square.flipped_vertically();
        }

        const int index = value_of(relative_piece_square) + value_of(piece.type()) * PIECE_TYPE_FACTOR +
                          (piece.color() == view) * PIECE_COLOR_FACTOR + king_square_idx * KING_SQUARE_FACTOR;

        return index;
    }

  private:
    static int king_square_index(const square_t relative_king_square)
    {

        // clang-format off
        constexpr enum_array<square_t, int> indices{
            0,  1,  2,  3,  3,  2,  1,  0,
            4,  5,  6,  7,  7,  6,  5,  4,
            8,  9,  10, 11, 11, 10, 9,  8,
            8,  9,  10, 11, 11, 10, 9,  8,
            12, 12, 13, 13, 13, 13, 12, 12,
            12, 12, 13, 13, 13, 13, 12, 12,
            14, 14, 15, 15, 15, 15, 14, 14,
            14, 14, 15, 15, 15, 15, 14, 14,
        };
        // clang-format on
        return indices[relative_king_square];
    }
};

struct WeightSource
{
    virtual ~WeightSource()                    = default;
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
    bool          swap_endian;

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

    virtual void parse_header() const       = 0;
    virtual void parse_layer_header() const = 0;

    template <typename Network>
    void load_network(Network& net) const
    {
        parse_header();
        auto layers = net.layers();
        std::apply([&](auto&... layer) { (load_one_layer(layer), ...); }, layers);
    }

  private:
    template <typename Layer>
    void load_one_layer(Layer& layer) const
    {
        parse_layer_header();
        layer.init_impl(m_deserializer);
    }
};

struct DotNetParser final : NnueParser
{

    explicit DotNetParser(const Deserializer& deserializer) : NnueParser(deserializer) {}

    void parse_header() const override {}

    void parse_layer_header() const override {}
};

template <typename T, std::size_t N>
struct DenseBuffer
{
    using value_type                    = T;
    static constexpr std::size_t size_v = N;

    std::array<T, N> data{};

    constexpr std::size_t size() const noexcept { return N; }
    T*                    data_ptr() noexcept { return data.data(); }
    const T*              data_ptr() const noexcept { return data.data(); }

    T&       operator[](std::size_t i) noexcept { return data[i]; }
    const T& operator[](std::size_t i) const noexcept { return data[i]; }
};

template <typename Derived, typename InputBufferT, typename OutputBufferT>
struct LayerBase
{
    using input_buffer_type  = InputBufferT;
    using output_buffer_type = OutputBufferT;

    static constexpr std::size_t input_size_v  = InputBufferT::size_v;
    static constexpr std::size_t output_size_v = OutputBufferT::size_v;

    void forward(const InputBufferT& in, OutputBufferT& out) const
    {
        static_cast<const Derived*>(this)->forward_impl(in, out);
    }

    template <typename Deserializer>
    void init(const Deserializer& d)
    {
        static_cast<Derived*>(this)->init_impl(d);
    }
};

template <typename T, std::size_t Size, std::size_t Scale>
struct Quantizer : LayerBase<Quantizer<T, Size, Scale>, DenseBuffer<T, Size>, DenseBuffer<T, Size>>
{
    using Input  = DenseBuffer<T, Size>;
    using Output = DenseBuffer<T, Size>;

    void forward_impl(const Input& in, Output& out) const
    {
        for (std::size_t i = 0; i < Size; ++i)
            out[i] = in[i] / static_cast<T>(Scale);
    }

    template <typename Deserializer>
    void init_impl(const Deserializer&)
    {
    }
};

template <typename InputT, typename OutputT, std::size_t InSz, std::size_t OutSz>
struct Dense : LayerBase<Dense<InputT, OutputT, InSz, OutSz>, DenseBuffer<InputT, InSz>, DenseBuffer<OutputT, OutSz>>
{
    using Input  = DenseBuffer<InputT, InSz>;
    using Output = DenseBuffer<OutputT, OutSz>;

    using WeightsT = std::array<InputT, InSz * OutSz>;
    using BiasT    = std::array<OutputT, OutSz>;

    std::unique_ptr<WeightsT> m_weights;
    std::unique_ptr<BiasT>    m_biases;

    void forward_impl(const Input& in, Output& out) const
    {
        for (std::size_t o = 0; o < OutSz; ++o)
        {
            OutputT sum = m_biases->data()[o];
            for (std::size_t i = 0; i < InSz; ++i)
                sum += static_cast<OutputT>(in[i]) * m_weights->data()[o * InSz + i];
            out[o] = sum;
        }
    }

    template <typename Deserializer>
    void init_impl(const Deserializer& d)
    {
        m_weights = std::make_unique<WeightsT>();
        m_biases  = std::make_unique<BiasT>();
        if (!m_weights || !m_biases)
        {
            throw std::runtime_error("Failed to allocate memory for deserialization");
        }
        for (size_t o = 0; o < OutSz; ++o)
            d.template read_array<InputT, InputT>(&m_weights->at(o * InSz), InSz);
        d.template read_array<OutputT, OutputT>(&m_biases->at(0), OutSz);
    }
};

template <typename T, std::size_t Size>
struct ReLU : LayerBase<ReLU<T, Size>, DenseBuffer<T, Size>, DenseBuffer<T, Size>>
{
    using Input  = DenseBuffer<T, Size>;
    using Output = DenseBuffer<T, Size>;

    void forward_impl(const Input& in, Output& out) const
    {
        for (std::size_t i = 0; i < Size; ++i)
            out[i] = std::max(T(0), in[i]);
    }

    template <typename Deserializer>
    void init_impl(const Deserializer&)
    {
    }
};

template <typename T, std::size_t Size, float ScaleFactor>
struct Sigmoid : LayerBase<Sigmoid<T, Size, ScaleFactor>, DenseBuffer<T, Size>, DenseBuffer<T, Size>>
{
    using Input  = DenseBuffer<T, Size>;
    using Output = DenseBuffer<T, Size>;

    void forward_impl(const Input& in, Output& out) const
    {
        for (std::size_t i = 0; i < Size; ++i)
            out[i] = T(1) / (T(1) + std::exp(-in[i] * ScaleFactor));
    }

    template <typename Deserializer>
    void init_impl(const Deserializer&)
    {
    }
};

template <typename T>
struct is_accumulator : std::false_type
{
};

template <typename T>
inline constexpr bool is_accumulator_v = is_accumulator<T>::value;

template <typename T, std::size_t OutSz, typename FeatureExtractorT>
struct Accumulator : LayerBase<Accumulator<T, OutSz, FeatureExtractorT>, color_t, DenseBuffer<T, 2 * OutSz>>
{
    using OutputT         = DenseBuffer<T, 2 * OutSz>;
    using OrientedOutputT = DenseBuffer<T, OutSz>;

    static constexpr std::size_t n_features = FeatureExtractorT::n_features;

    using WeightsT = std::array<T, OutSz * n_features>;
    using BiasT    = std::array<T, OutSz>;

    std::shared_ptr<WeightsT> m_weights;
    std::shared_ptr<BiasT>    m_biases;

    using StateT = OrientedOutputT;
    enum_array<color_t, StateT> m_state;

    using FeatureT = FeatureExtractorT::FeatureT;

    Accumulator() = default;

    Accumulator(const Accumulator& other)
        : m_weights(std::move(other.m_weights)), m_biases(std::move(other.m_biases)), m_state(other.m_state)
    {
    }

    template <color_t view>
    void add_feature(const position_t& pos, const square_t piece_square, const piece_t pc)
    {
        add_feature_impl<view>(FeatureExtractorT::template extract<view>(pos, piece_square, pc));
    }

    void add_feature(const position_t& pos, const square_t piece_square, const piece_t pc)
    {
        add_feature<WHITE>(pos, piece_square, pc);
        add_feature<BLACK>(pos, piece_square, pc);
    }

    template <color_t view>
    void remove_feature(const position_t& pos, const square_t piece_square, const piece_t pc)
    {
        remove_feature_impl<view>(FeatureExtractorT::template extract<view>(pos, piece_square, pc));
    }

    void remove_feature(const position_t& pos, const square_t piece_square, const piece_t pc)
    {
        remove_feature<WHITE>(pos, piece_square, pc);
        remove_feature<BLACK>(pos, piece_square, pc);
    }

    template <color_t view>
    void refresh(const position_t& pos)
    {
        std::memcpy(m_state.at(view).data_ptr(), m_biases->data(), OutSz * sizeof(T));
        bitboard_t pieces = pos.occupancy();
        while (pieces)
        {
            const square_t sq{pieces.pops_lsb()};
            add_feature<view>(pos, sq, pos.piece_at(sq));
        }
    }

    void forward_impl(const color_t& c, OutputT& out) const
    {
        for (std::size_t i = 0; i < OutSz; ++i)
        {
            out[i]         = m_state.at(c)[i];
            out[i + OutSz] = m_state.at(~c)[i];
        }
    }

    template <typename Deserializer>
    void init_impl(const Deserializer& d)
    {
        m_weights = std::make_unique<WeightsT>();
        m_biases  = std::make_unique<BiasT>();
        if (!m_weights || !m_biases)
        {
            throw std::runtime_error("Failed to allocate memory for deserialization");
        }
        for (size_t o = 0; o < n_features; ++o)
            d.template read_array<T, T>(&m_weights->at(o * OutSz), OutSz);
        d.template read_array<T, T>(&m_biases->at(0), OutSz);
    }

  private:
    template <color_t view>
    void add_feature_impl(const FeatureT idx)
    {
        for (std::size_t o = 0; o < OutSz; ++o)
            m_state.at(view)[o] += m_weights->data()[idx * OutSz + o];
    }

    template <color_t view>
    void remove_feature_impl(const FeatureT idx)
    {
        for (std::size_t o = 0; o < OutSz; ++o)
            m_state.at(view)[o] -= m_weights->data()[idx * OutSz + o];
    }
};

template <typename OutT, std::size_t OutSz, typename FeatureExtractorT>
struct is_accumulator<Accumulator<OutT, OutSz, FeatureExtractorT>> : std::true_type
{
};

template <typename AccumulatorT, typename... LayerTs>
struct NNUE
{
    static_assert((std::is_class_v<LayerTs> && ...), "All LayerTs must be class types");
    static_assert(is_accumulator_v<AccumulatorT>, "First template arg must be an accumulator");

    using AccBufT = typename AccumulatorT::output_buffer_type;

    explicit NNUE(LayerTs... ls)
        : m_layers(std::move(ls)...)
    {
        m_accumulator_stack.emplace_back();
    }

    auto layers()
    {
        return std::tuple_cat(std::forward_as_tuple(accumulator()),
                              std::apply([](auto&... layers) {
                                  return std::forward_as_tuple(layers...);
                              }, m_layers));
    }

    auto layers() const
    {
        return std::tuple_cat(std::forward_as_tuple(accumulator()),
                              std::apply([](auto const&... layers) {
                                  return std::forward_as_tuple(layers...);
                              }, m_layers));
    }

    AccumulatorT&       accumulator()       { return m_accumulator_stack.back(); }
    const AccumulatorT& accumulator() const { return m_accumulator_stack.back(); }

    template <typename LastOut>
    void forward(color_t c, LastOut& out) const
    {
        AccBufT acc_out;
        m_accumulator_stack.back().forward(c, acc_out);
        forward_impl<0>(acc_out, out);
    }

    template <color_t us>
    void do_move(const position_t& pos, const move_t move)
    {
        push_state();
        constexpr color_t them = ~us;
        switch (move.type_of())
        {
            case CASTLING:
            {
                auto [k_from, k_to] = move.castling_type().king_move();
                auto [r_from, r_to] = move.castling_type().rook_move();
                accumulator().template add_feature<them>(pos, r_to, piece_t{us, ROOK});
                accumulator().template remove_feature<them>(pos, r_from, piece_t{us, ROOK});
                accumulator().template add_feature<them>(pos, k_to, piece_t{us, KING});
                accumulator().template remove_feature<them>(pos, k_from, piece_t{us, KING});
                accumulator().template refresh<us>(pos);
                break;
            }
            case EN_PASSANT:
            {
                constexpr direction_t down = relative_dir<us, SOUTH>;
                accumulator().remove_feature(pos, move.from_sq(), piece_t{us, PAWN});
                accumulator().add_feature(pos, move.to_sq(), piece_t{us, PAWN});
                accumulator().remove_feature(pos, move.to_sq() - down, piece_t{them, PAWN});
                break;
            }
            case NORMAL:
            {
                if (pos.piece_at(move.to_sq()).type() == KING)
                {
                    accumulator().template refresh<us>(pos);
                    accumulator().template add_feature<them>(pos, move.to_sq(), piece_t{us, KING});
                    accumulator().template remove_feature<them>(pos, move.from_sq(), piece_t{us, KING});
                    if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                    {
                        accumulator().template remove_feature<them>(pos, move.to_sq(), taken);
                    }
                }
                else
                {
                    accumulator().add_feature(pos, move.to_sq(), pos.piece_at(move.to_sq()));
                    accumulator().remove_feature(pos, move.from_sq(), pos.piece_at(move.to_sq()));
                    if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                    {
                        accumulator().remove_feature(pos, move.to_sq(), taken);
                    }
                }
                break;
            }
            case PROMOTION:
            {
                accumulator().add_feature(pos, move.to_sq(), piece_t{us, move.promotion_type()});
                accumulator().remove_feature(pos, move.from_sq(), piece_t{us, PAWN});
                if (const piece_t taken = pos.taken(); taken != NO_PIECE)
                {
                    accumulator().remove_feature(pos, move.to_sq(), taken);
                }
                break;
            }
            default:
                break;
        }
    }

    void undo_move()
    {
        pop_state();
    }

private:
    void push_state() { m_accumulator_stack.emplace_back(accumulator()); }

    void pop_state()
    {
        if (m_accumulator_stack.empty())
            throw std::runtime_error("Accumulator stack underflow");
        m_accumulator_stack.pop_back();
    }

    template <std::size_t Index, typename InBuf, typename OutBuf>
    void forward_impl(const InBuf& in, OutBuf& out) const
    {
        using LayerT = std::tuple_element_t<Index, std::tuple<LayerTs...>>;
        static_assert(LayerT::input_size_v == InBuf::size_v, "Input size mismatch");

        auto&                               layer = std::get<Index>(m_layers);
        typename LayerT::output_buffer_type tmp{};
        layer.forward(in, tmp);

        if constexpr (Index + 1 == sizeof...(LayerTs))
        {
            static_assert(LayerT::output_size_v == OutBuf::size_v, "Output size mismatch");
            out = tmp;
        }
        else
        {
            forward_impl<Index + 1>(tmp, out);
        }
    }

    std::tuple<LayerTs...> m_layers;

    std::vector<AccumulatorT> m_accumulator_stack{256};
};

namespace Koi
{
    using FeatureExtractorT = KoiFeatureExtractor;
    using AccumulatorT      = Accumulator<int16_t, 512, FeatureExtractorT>;
    using ReluT             = ReLU<int16_t, 1024>;
    using DenseT            = Dense<int16_t, int32_t, 1024, 1>;
    using QuantT            = Quantizer<int32_t, 1, 32 * 128>;

    using Net = NNUE<AccumulatorT, ReluT, DenseT, QuantT>;
} // namespace Koi

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
