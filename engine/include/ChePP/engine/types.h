#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <expected>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>

using hash_t = std::uint64_t;

template <typename T>
struct enum_utils;

template <typename T, typename = void>
struct has_enum_utils : std::false_type
{
};

template <typename T>
struct has_enum_utils<T, std::void_t<decltype(enum_utils<T>::is_well_ordered)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_enum_utils_v = has_enum_utils<T>::value;

#define DEFINE_HAS_ENUM_UTILS_MEMBER(NAME)                                                                             \
    template <typename T, typename = void>                                                                             \
    struct has_##NAME : std::false_type                                                                                \
    {                                                                                                                  \
    };                                                                                                                 \
                                                                                                                       \
    template <typename T>                                                                                              \
    struct has_##NAME<T, std::void_t<decltype(&enum_utils<T>::NAME)>> : std::true_type                                 \
    {                                                                                                                  \
    };                                                                                                                 \
                                                                                                                       \
    template <typename T>                                                                                              \
    inline constexpr bool has_##NAME##_v = has_##NAME<T>::value;

DEFINE_HAS_ENUM_UTILS_MEMBER(to_string)
DEFINE_HAS_ENUM_UTILS_MEMBER(from_string)
DEFINE_HAS_ENUM_UTILS_MEMBER(to_char)
DEFINE_HAS_ENUM_UTILS_MEMBER(from_char)
DEFINE_HAS_ENUM_UTILS_MEMBER(index)
DEFINE_HAS_ENUM_UTILS_MEMBER(value_at) // ordinal
DEFINE_HAS_ENUM_UTILS_MEMBER(value_of) // actual value
DEFINE_HAS_ENUM_UTILS_MEMBER(repr)
DEFINE_HAS_ENUM_UTILS_MEMBER(values)

template <typename enum_type, std::enable_if_t<has_enum_utils_v<enum_type>, int> = 0>
constexpr bool is_well_ordered = enum_utils<enum_type>::is_well_ordered;

template <typename enum_type, std::enable_if_t<has_enum_utils_v<enum_type>, int> = 0>
constexpr enum_utils<enum_type>::idx_type count = enum_utils<enum_type>::count;

template <typename Enum, std::size_t... Is>
constexpr std::array<Enum, sizeof...(Is)> generate_enum_values(std::index_sequence<Is...>)
{
    return {Enum(Is)...};
}

template <typename Enum, std::enable_if_t<has_enum_utils_v<Enum>, int> = 0>
constexpr std::array<Enum, count<Enum>> make_values()
{
    if constexpr (!has_values_v<Enum> && is_well_ordered<Enum>)
    {
        return generate_enum_values<Enum>(std::make_index_sequence<count<Enum>>{});
    }
    else
    {
        return enum_utils<Enum>::values;
    }
}

template <typename enum_type, std::enable_if_t<has_enum_utils_v<enum_type>, int> = 0>
constexpr std::array<enum_type, enum_utils<enum_type>::count> values = make_values<enum_type>();

template <typename enum_type, std::enable_if_t<has_enum_utils_v<enum_type>, int> = 0>
using repr_type = std::array<std::string_view, enum_utils<enum_type>::count>;

template <typename enum_type, std::enable_if_t<has_enum_utils_v<enum_type>, int> = 0>
constexpr repr_type<enum_type> repr = enum_utils<enum_type>::repr;

template <typename enum_type>
[[nodiscard]] constexpr enum_type value_at(const typename enum_utils<enum_type>::idx_type idx)
{
    return values<enum_type>.at(idx);
}

template <typename enum_type>
[[nodiscard]] constexpr enum_utils<enum_type>::underlying_type value_of(const enum_type val)
{
    return enum_utils<enum_type>::value_of(val);
}

template <typename enum_type>
[[nodiscard]] constexpr enum_utils<enum_type>::idx_type index(const enum_type value)
{
    if constexpr (is_well_ordered<enum_type>)
    {
        return static_cast<enum_utils<enum_type>::idx_type>(value_of(value));
    }
    else
    {
        return enum_utils<enum_type>::index(value);
    }
}

template <typename enum_type, std::enable_if_t<has_to_string_v<enum_type>, int> = 0>
[[nodiscard]] constexpr decltype(auto) to_string(const enum_type value)
{
    return enum_utils<enum_type>::to_string(value);
}

template <typename enum_type, std::enable_if_t<!has_to_string_v<enum_type> && has_repr_v<enum_type>, int> = 0>
[[nodiscard]] constexpr decltype(auto) to_string(const enum_type value)
{
    return repr<enum_type>.at(index(value));
}

template <typename enum_type, std::enable_if_t<has_from_string_v<enum_type>, int> = 0>
[[nodiscard]] constexpr std::optional<enum_type> from_string(const std::string_view& value)
{
    return enum_utils<enum_type>::from_string(value);
}

// slow fallback, please define from string if possible
template <typename enum_type, std::enable_if_t<!has_from_string_v<enum_type> && has_repr_v<enum_type>, int> = 0>
[[nodiscard]] constexpr std::optional<enum_type> from_string(const std::string_view& value)
{
    const auto it = std::find(repr<enum_type>.begin(), repr<enum_type>.end(), value);
    if (it == repr<enum_type>.end())
        return std::nullopt;
    return values<enum_type>.at(std::distance(repr<enum_type>.begin(), it));
}

template <typename enum_type>
[[nodiscard]] constexpr char to_char(const enum_type value)
{
    return to_string<enum_type>(value).at(0);
}

template <typename enum_type>
[[nodiscard]] constexpr std::optional<enum_type> from_char(const char value)
{
    return from_string<enum_type>(std::string_view{&value, 1});
}

template <typename enum_type, std::enable_if_t<has_to_string_v<enum_type> || has_repr_v<enum_type>, int> = 0>
std::ostream& operator<<(std::ostream& os, const enum_type value)
{
    return os << to_string<enum_type>(value);
}

// Unary minus
template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr enum_type operator-(const enum_type rhs)
{
    return enum_type{-value_of(rhs)};
}

// Pre-increment
template <typename enum_type, std::enable_if_t<is_well_ordered<enum_type>, int> = 0>
constexpr enum_type& operator++(enum_type& rhs)
{
    rhs = enum_type{value_of(rhs) + 1};
    return rhs;
}

template <typename enum_type, std::enable_if_t<is_well_ordered<enum_type>, int> = 0>
constexpr enum_type& operator--(enum_type& rhs)
{
    rhs = enum_type{value_of(rhs) - 1};
    return rhs;
}

// Post-increment
template <typename enum_type, std::enable_if_t<is_well_ordered<enum_type>, int> = 0>
constexpr enum_type operator++(enum_type& rhs, int)
{
    enum_type tmp = rhs;
    rhs           = enum_type{value_of(rhs) + 1};
    return tmp;
}

template <typename enum_type, std::enable_if_t<is_well_ordered<enum_type>, int> = 0>
constexpr enum_type operator--(enum_type& rhs, int)
{
    enum_type tmp = rhs;
    rhs           = enum_type{value_of(rhs) - 1};
    return tmp;
}

// Enum + Enum
template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr enum_type operator+(const enum_type lhs, const enum_type rhs)
{
    return enum_type{value_of(lhs) + value_of(rhs)};
}

// Enum + int
template <typename enum_type, typename int_type,
          std::enable_if_t<has_value_of_v<enum_type> && std::is_signed_v<int_type>, int> = 0>
constexpr enum_type operator+(const enum_type lhs, const int_type rhs)
{
    return enum_type{value_of(lhs) + rhs};
}

// int + Enum
template <typename int_type, typename enum_type,
          std::enable_if_t<has_value_of_v<enum_type> && std::is_signed_v<int_type>, int> = 0>
constexpr enum_type operator+(const int_type lhs, const enum_type rhs)
{
    return rhs + lhs;
}

// Enum - Enum
template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr enum_type operator-(const enum_type lhs, const enum_type rhs)
{
    return enum_type{value_of(lhs) - value_of(rhs)};
}

// Enum - int
template <typename enum_type, typename int_type,
          std::enable_if_t<has_value_of_v<enum_type> && std::is_signed_v<int_type>, int> = 0>
constexpr enum_type operator-(const enum_type lhs, const int_type rhs)
{
    return enum_type{value_of(lhs) - rhs};
}

// int - Enum
template <typename int_type, typename enum_type,
          std::enable_if_t<has_value_of_v<enum_type> && std::is_signed_v<int_type>, int> = 0>
constexpr enum_type operator-(const int_type lhs, const enum_type rhs)
{
    return enum_type{lhs - value_of(rhs)};
}

// Enum * int
template <typename enum_type, typename int_type,
          std::enable_if_t<has_value_of_v<enum_type> && std::is_signed_v<int_type>, int> = 0>
constexpr enum_type operator*(const enum_type lhs, const int_type rhs)
{
    return enum_type{value_of(lhs) * rhs};
}

// int * Enum
template <typename int_type, typename enum_type,
          std::enable_if_t<has_value_of_v<enum_type> && std::is_signed_v<int_type>, int> = 0>
constexpr enum_type operator*(const int_type lhs, const enum_type rhs)
{
    return rhs * lhs;
}

// Equality
template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr bool operator==(const enum_type lhs, const enum_type rhs)
{
    return value_of(lhs) == value_of(rhs);
}

template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr bool operator!=(const enum_type lhs, const enum_type rhs)
{
    return value_of(lhs) != value_of(rhs);
}

// Comparisons
template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr bool operator<(const enum_type lhs, const enum_type rhs)
{
    return value_of(lhs) < value_of(rhs);
}

template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr bool operator<=(const enum_type lhs, const enum_type rhs)
{
    return value_of(lhs) <= value_of(rhs);
}

template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr bool operator>(const enum_type lhs, const enum_type rhs)
{
    return value_of(lhs) > value_of(rhs);
}

template <typename enum_type, std::enable_if_t<has_value_of_v<enum_type>, int> = 0>
constexpr bool operator>=(const enum_type lhs, const enum_type rhs)
{
    return value_of(lhs) >= value_of(rhs);
}

template <typename enum_type, typename value_type>
struct enum_array
{
    static_assert(is_well_ordered<enum_type>, "Enum must be iterable for enum_array");

    static constexpr size_t count = enum_utils<enum_type>::count;

    std::array<value_type, count> m_data;

    constexpr value_type& operator[](enum_type e) noexcept { return m_data[enum_utils<enum_type>::index(e)]; }

    constexpr const value_type& operator[](enum_type e) const noexcept { return m_data.at(index<enum_type>(e)); }

    constexpr value_type& at(enum_type e) { return m_data.at(index(e)); }

    [[nodiscard]] constexpr const value_type& at(enum_type e) const { return m_data.at(index<enum_type>(e)); }

    [[nodiscard]] static constexpr size_t size() noexcept { return count; }

    constexpr auto               begin() noexcept { return m_data.begin(); }
    constexpr auto               end() noexcept { return m_data.end(); }
    [[nodiscard]] constexpr auto begin() const noexcept { return m_data.begin(); }
    [[nodiscard]] constexpr auto end() const noexcept { return m_data.end(); }

    [[nodiscard]] constexpr auto values() const noexcept { return enum_utils<enum_type>::values; }

    constexpr void fill(const value_type& value) { m_data.fill(value); }

    template <typename Func>
    constexpr void for_each(Func&& f)
    {
        for (size_t i = 0; i < count; ++i)
        {
            f(enum_utils<enum_type>::values[i], m_data[i]);
        }
    }

    template <typename Func>
    constexpr void for_each(Func&& f) const
    {
        for (size_t i = 0; i < count; ++i)
        {
            f(enum_utils<enum_type>::values[i], m_data[i]);
        }
    }
};

struct file_t
{
    using value_type = int8_t;

    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit file_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val >= 0 && val <= 8);
    }

    value_type m_val;
    friend enum_utils<file_t>;
};

constexpr file_t FILE_A{0};
constexpr file_t FILE_B{1};
constexpr file_t FILE_C{2};
constexpr file_t FILE_D{3};
constexpr file_t FILE_E{4};
constexpr file_t FILE_F{5};
constexpr file_t FILE_G{6};
constexpr file_t FILE_H{7};
constexpr file_t NO_FILE{8};

template <>
struct enum_utils<file_t>
{
    using enum_type       = file_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 8;

    //clang-format off
    static constexpr std::array           values = {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
    static constexpr repr_type<enum_type> repr   = {"a", "b", "c", "d", "e", "f", "g", "h"};
    //clang-format on

    static constexpr underlying_type value_of(const enum_type val) { return val.m_val; }

    static constexpr std::optional<enum_type> from_string(const std::string_view& value)
    {
        if (value.length() != 1)
            return std::nullopt;
        const char c = value[0];
        if (c < 'a' || c > 'h')
            return std::nullopt;
        return values.at(c - 'a');
    }
};

struct rank_t
{
    using value_type = int8_t;

    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit rank_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val >= 0 && val <= 8);
    }

    value_type m_val;
    friend enum_utils<rank_t>;
};

constexpr rank_t RANK_1{0};
constexpr rank_t RANK_2{1};
constexpr rank_t RANK_3{2};
constexpr rank_t RANK_4{3};
constexpr rank_t RANK_5{4};
constexpr rank_t RANK_6{5};
constexpr rank_t RANK_7{6};
constexpr rank_t RANK_8{7};
constexpr rank_t NO_RANK{8};

template <>
struct enum_utils<rank_t>
{
    using enum_type       = rank_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 8;

    //clang-format off
    static constexpr std::array           values = {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};
    static constexpr repr_type<enum_type> repr   = {"1", "2", "3", "4", "5", "6", "7", "8"};
    //clang-format on

    static constexpr underlying_type value_of(const enum_type sq) { return sq.m_val; }

    static constexpr std::optional<enum_type> from_string(const std::string_view& value)
    {
        if (value.length() != 1)
            return std::nullopt;
        const char c = value[0];
        if (c < '1' || c > '8')
            return std::nullopt;
        return values.at(c - '1');
    }
};

using coordinates_t = std::pair<file_t, rank_t>;

struct square_t;
template <>
struct enum_utils<square_t>;

struct square_t
{
    using value_type = int8_t;

    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit square_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val >= 0 && val <= 64);
    }

    constexpr explicit square_t(const coordinates_t& coordinates)
        : m_val(static_cast<value_type>(index(coordinates.first) +
                                        index(coordinates.second) * count<decltype(coordinates.second)>))
    {
        assert(m_val >= 0 && m_val <= 63);
    }

    constexpr explicit square_t(const file_t file, const rank_t rank) : square_t({file, rank}) {}

    [[nodiscard]] constexpr file_t file() const noexcept { return file_t{m_val & 7}; }

    [[nodiscard]] constexpr rank_t rank() const noexcept { return rank_t{m_val >> 3}; }

    [[nodiscard]] constexpr coordinates_t coordinates() const noexcept { return {file(), rank()}; }

    [[nodiscard]] constexpr square_t flipped_horizontally() const noexcept { return square_t{file(), RANK_8 - rank()}; }

    [[nodiscard]] constexpr square_t flipped_vertically() const noexcept { return square_t{FILE_H - file(), rank()}; }


    value_type m_val;
    friend enum_utils<square_t>;
};

constexpr square_t A1{{FILE_A, RANK_1}};
constexpr square_t A2{{FILE_A, RANK_2}};
constexpr square_t A3{{FILE_A, RANK_3}};
constexpr square_t A4{{FILE_A, RANK_4}};
constexpr square_t A5{{FILE_A, RANK_5}};
constexpr square_t A6{{FILE_A, RANK_6}};
constexpr square_t A7{{FILE_A, RANK_7}};
constexpr square_t A8{{FILE_A, RANK_8}};

constexpr square_t B1{{FILE_B, RANK_1}};
constexpr square_t B2{{FILE_B, RANK_2}};
constexpr square_t B3{{FILE_B, RANK_3}};
constexpr square_t B4{{FILE_B, RANK_4}};
constexpr square_t B5{{FILE_B, RANK_5}};
constexpr square_t B6{{FILE_B, RANK_6}};
constexpr square_t B7{{FILE_B, RANK_7}};
constexpr square_t B8{{FILE_B, RANK_8}};

constexpr square_t C1{{FILE_C, RANK_1}};
constexpr square_t C2{{FILE_C, RANK_2}};
constexpr square_t C3{{FILE_C, RANK_3}};
constexpr square_t C4{{FILE_C, RANK_4}};
constexpr square_t C5{{FILE_C, RANK_5}};
constexpr square_t C6{{FILE_C, RANK_6}};
constexpr square_t C7{{FILE_C, RANK_7}};
constexpr square_t C8{{FILE_C, RANK_8}};

constexpr square_t D1{{FILE_D, RANK_1}};
constexpr square_t D2{{FILE_D, RANK_2}};
constexpr square_t D3{{FILE_D, RANK_3}};
constexpr square_t D4{{FILE_D, RANK_4}};
constexpr square_t D5{{FILE_D, RANK_5}};
constexpr square_t D6{{FILE_D, RANK_6}};
constexpr square_t D7{{FILE_D, RANK_7}};
constexpr square_t D8{{FILE_D, RANK_8}};

constexpr square_t E1{{FILE_E, RANK_1}};
constexpr square_t E2{{FILE_E, RANK_2}};
constexpr square_t E3{{FILE_E, RANK_3}};
constexpr square_t E4{{FILE_E, RANK_4}};
constexpr square_t E5{{FILE_E, RANK_5}};
constexpr square_t E6{{FILE_E, RANK_6}};
constexpr square_t E7{{FILE_E, RANK_7}};
constexpr square_t E8{{FILE_E, RANK_8}};

constexpr square_t F1{{FILE_F, RANK_1}};
constexpr square_t F2{{FILE_F, RANK_2}};
constexpr square_t F3{{FILE_F, RANK_3}};
constexpr square_t F4{{FILE_F, RANK_4}};
constexpr square_t F5{{FILE_F, RANK_5}};
constexpr square_t F6{{FILE_F, RANK_6}};
constexpr square_t F7{{FILE_F, RANK_7}};
constexpr square_t F8{{FILE_F, RANK_8}};

constexpr square_t G1{{FILE_G, RANK_1}};
constexpr square_t G2{{FILE_G, RANK_2}};
constexpr square_t G3{{FILE_G, RANK_3}};
constexpr square_t G4{{FILE_G, RANK_4}};
constexpr square_t G5{{FILE_G, RANK_5}};
constexpr square_t G6{{FILE_G, RANK_6}};
constexpr square_t G7{{FILE_G, RANK_7}};
constexpr square_t G8{{FILE_G, RANK_8}};

constexpr square_t H1{{FILE_H, RANK_1}};
constexpr square_t H2{{FILE_H, RANK_2}};
constexpr square_t H3{{FILE_H, RANK_3}};
constexpr square_t H4{{FILE_H, RANK_4}};
constexpr square_t H5{{FILE_H, RANK_5}};
constexpr square_t H6{{FILE_H, RANK_6}};
constexpr square_t H7{{FILE_H, RANK_7}};
constexpr square_t H8{{FILE_H, RANK_8}};

constexpr square_t NO_SQUARE{64};

template <>
struct enum_utils<square_t>
{
    using enum_type       = square_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 65;

    //clang-format off
    static constexpr repr_type<enum_type> repr = {
        "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "a3",
        "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a5", "b5",
        "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "a7", "b7", "c7",
        "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "-"};
    //clang-format on

    static constexpr idx_type value_of(const enum_type sq) { return static_cast<idx_type>(sq.m_val); }

    static constexpr std::optional<enum_type> from_string(const std::string_view& value)
    {
        if (value.length() == 1 && value[0] == '-')
        {
            return NO_SQUARE;
        }
        if (value.length() == 2)
        {
            const auto file = from_char<file_t>(value.at(0));
            const auto rank = from_char<rank_t>(value.at(1));

            if (file && rank)
            {
                return square_t{{file.value(), rank.value()}};
            }
        }
        return std::nullopt;
    }
};

struct piece_type_t
{
    using value_type = int8_t;

    constexpr piece_type_t() : m_val(0) {};

    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit piece_type_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val >= 0 && val <= 7);
    }

    using piece_value_type = int;
    [[nodiscard]] constexpr piece_value_type value() const;

    value_type m_val;
    friend enum_utils<piece_type_t>;
};

constexpr piece_type_t PAWN{0};
constexpr piece_type_t KNIGHT{1};
constexpr piece_type_t BISHOP{2};
constexpr piece_type_t ROOK{3};
constexpr piece_type_t QUEEN{4};
constexpr piece_type_t KING{5};
constexpr piece_type_t NO_PIECE_TYPE{6};

template <>
struct enum_utils<piece_type_t>
{
    using enum_type       = piece_type_t;
    using idx_type        = uint8_t;
    using underlying_type = int8_t;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 7;

    static constexpr repr_type<enum_type> repr = {"p", "n", "b", "r", "q", "k", "-"};

    static constexpr underlying_type value_of(const enum_type val) { return val.m_val; }
};

inline enum_array<piece_type_t, int> piece_type_value{100, 300, 325, 500, 900, 1000};

[[nodiscard]] constexpr piece_type_t::piece_value_type piece_type_t::value() const
{
    return piece_type_value.at(*this);
}

struct color_t
{
    using value_type = uint8_t;

    constexpr color_t() : m_val(0) {};
    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit color_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val == 0 || val == 1);
    }

    constexpr color_t operator!() const { return color_t(static_cast<value_type>(m_val ^ 1)); }
    constexpr color_t operator~() const { return !(*this); }

    value_type m_val;
    friend enum_utils<color_t>;
};

constexpr color_t WHITE{0};
constexpr color_t BLACK{1};

template <>
struct enum_utils<color_t>
{
    using enum_type       = color_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 2;

    static constexpr repr_type<enum_type> repr = {"w", "b"};

    static constexpr underlying_type value_of(const enum_type val) { return val.m_val; }

    static constexpr std::optional<enum_type> from_string(const std::string_view& value)
    {
        if (value == "w")
            return WHITE;
        if (value == "b")
            return BLACK;
        return std::nullopt;
    }
};

struct piece_t
{
    using value_type = uint8_t;

    constexpr piece_t() : m_val(12) {}
    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit piece_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val >= 0 && val <= 12);
    }
    explicit constexpr piece_t(const color_t c, const piece_type_t pt) : piece_t(index(c) + (index(pt) << 1)) {}

    [[nodiscard]] constexpr piece_type_t type() const { return value_at<piece_type_t>(m_val >> 1); }

    [[nodiscard]] constexpr color_t color() const { return value_at<color_t>(m_val & 1); }

    using piece_value_t = int;
    [[nodiscard]] constexpr piece_value_t value() const { return piece_type_value.at(this->type()); }

  private:
    value_type m_val;
    friend enum_utils<piece_t>;
};

constexpr piece_t W_PAWN{WHITE, PAWN};
constexpr piece_t W_KNIGHT{WHITE, KNIGHT};
constexpr piece_t W_BISHOP{WHITE, BISHOP};
constexpr piece_t W_ROOK{WHITE, ROOK};
constexpr piece_t W_QUEEN{WHITE, QUEEN};
constexpr piece_t W_KING{WHITE, KING};
constexpr piece_t B_PAWN{BLACK, PAWN};
constexpr piece_t B_KNIGHT{BLACK, KNIGHT};
constexpr piece_t B_BISHOP{BLACK, BISHOP};
constexpr piece_t B_ROOK{BLACK, ROOK};
constexpr piece_t B_QUEEN{BLACK, QUEEN};
constexpr piece_t B_KING{BLACK, KING};
constexpr piece_t NO_PIECE{12};

template <>
struct enum_utils<piece_t>
{
    using enum_type       = piece_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 13;

    static constexpr repr_type<piece_t> repr = {"P", "p", "N", "n", "B", "b", "R", "r", "Q", "q", "K", "k", "-"};

    static constexpr underlying_type value_of(const enum_type val) { return val.m_val; }
};

struct direction_t
{
    using value_type = int8_t;

    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type> && std::is_signed_v<int_type>, int> = 0>
    constexpr explicit direction_t(int_type val) : m_offset(static_cast<value_type>(val))
    {
        assert(val >= -64 && val <= 64);
    }

    value_type m_offset;
};

constexpr direction_t NORTH{8};
constexpr direction_t EAST{1};
constexpr direction_t SOUTH{-8};
constexpr direction_t WEST{-1};
constexpr direction_t NORTH_EAST{8 + 1};
constexpr direction_t NORTH_WEST{8 - 1};
constexpr direction_t SOUTH_EAST{-8 + 1};
constexpr direction_t SOUTH_WEST{-8 - 1};
constexpr direction_t NO_DIRECTION{0};

template <>
struct enum_utils<direction_t>
{
    using enum_type       = direction_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = false;
    static constexpr idx_type count           = 9;

    //clang-format off
    static constexpr std::array<enum_type, count> values = {NORTH,      NORTH_EAST, EAST,       SOUTH_EAST,  SOUTH,
                                                            SOUTH_WEST, WEST,       NORTH_WEST, NO_DIRECTION};
    //clang-format on

    static constexpr underlying_type value_of(const enum_type val) { return val.m_offset; }

    static constexpr idx_type index(const enum_type dir)
    {
        switch (dir.m_offset)
        {
            case NORTH.m_offset:
                return 0;
            case NORTH_EAST.m_offset:
                return 1;
            case EAST.m_offset:
                return 2;
            case SOUTH_EAST.m_offset:
                return 3;
            case SOUTH.m_offset:
                return 4;
            case SOUTH_WEST.m_offset:
                return 5;
            case WEST.m_offset:
                return 6;
            case NORTH_WEST.m_offset:
                return 7;
            case NO_DIRECTION.m_offset:
                return 8;
            default:
                assert(false);
        }
        assert(false);
        return 0;
    }
};

constexpr square_t operator+(const square_t s, const direction_t d)
{
    return square_t{s + value_of(d)};
}

constexpr square_t operator-(const square_t s, const direction_t d)
{
    return square_t{s - value_of(d)};
}

template <direction_t d>
constexpr direction_t inverse_dir = -d;

template <color_t c, direction_t d>
constexpr direction_t relative_dir = c == WHITE ? d : inverse_dir<d>;

template <color_t c, rank_t r>
constexpr rank_t relative_rank = c == WHITE ? r : RANK_8 - r;

template <color_t c, file_t f>
constexpr file_t relative_file = f;

template <color_t c, square_t sq>
constexpr auto relative_square = square_t{relative_file<c, sq.file()>, relative_rank<c, sq.rank()>};

constexpr direction_t direction_from(const square_t a, const square_t b)
{
    const int dr = value_of(b.rank()) - value_of(a.rank());
    const int df = value_of(b.file()) - value_of(a.file());

    if (dr == 0 && df > 0)
        return EAST;
    if (dr == 0 && df < 0)
        return WEST;
    if (df == 0 && dr > 0)
        return NORTH;
    if (df == 0 && dr < 0)
        return SOUTH;
    if (dr == df && dr > 0)
        return SOUTH_EAST;
    if (dr == df && dr < 0)
        return SOUTH_WEST;
    if (dr == -df && dr > 0)
        return NORTH_WEST;
    if (dr == -df && dr < 0)
        return SOUTH_EAST;

    return NO_DIRECTION;
}

enum castling_side_t : uint8_t
{
    KINGSIDE  = 0,
    QUEENSIDE = 1,
};

struct castling_type_t
{
    using value_type = uint8_t;

    template <typename int_type, std::enable_if_t<std::is_integral_v<int_type>, int> = 0>
    constexpr explicit castling_type_t(int_type val) : m_val(static_cast<value_type>(val))
    {
        assert(val >= 0 && val <= 4);
    }

    constexpr explicit castling_type_t(const color_t c, const castling_side_t side)
        : castling_type_t((value_of(c) << 1) + side)
    {
    }

    [[nodiscard]] constexpr value_type mask() const { return 1 << m_val; }

    [[nodiscard]] constexpr std::pair<square_t, square_t> king_move() const;
    [[nodiscard]] constexpr std::pair<square_t, square_t> rook_move() const;

    value_type m_val;
    friend enum_utils<castling_side_t>;
};

constexpr castling_type_t WHITE_KINGSIDE{WHITE, KINGSIDE};
constexpr castling_type_t BLACK_KINGSIDE{BLACK, KINGSIDE};
constexpr castling_type_t WHITE_QUEENSIDE{WHITE, QUEENSIDE};
constexpr castling_type_t BLACK_QUEENSIDE{BLACK, QUEENSIDE};
constexpr castling_type_t NO_CASTLING_TYPE{4};

template <>
struct enum_utils<castling_type_t>
{
    using enum_type       = castling_type_t;
    using idx_type        = uint8_t;
    using underlying_type = enum_type::value_type;

    static constexpr bool     is_well_ordered = true;
    static constexpr idx_type count           = 5;

    static constexpr repr_type<enum_type> repr = {"K", "Q", "k", "q", "-"};

    static constexpr underlying_type value_of(const enum_type val) { return val.m_val; }
};

constexpr enum_array<castling_type_t, std::pair<square_t, square_t>> king_moves{
    std::pair{E1, G1}, {E1, C1}, {E8, G8}, {E8, C8}, {NO_SQUARE, NO_SQUARE}};

constexpr std::pair<square_t, square_t> castling_type_t::king_move() const
{
    return king_moves.at(*this);
}

constexpr enum_array<castling_type_t, std::pair<square_t, square_t>> rook_moves{
    std::pair{H1, F1}, {A1, D1}, {H8, F8}, {A8, D8}, {NO_SQUARE, NO_SQUARE}};
constexpr std::pair<square_t, square_t> castling_type_t::rook_move() const
{
    return rook_moves.at(*this);
}

enum move_type_t : uint16_t
{
    NORMAL     = 0,
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14
};

// a move is encoded as an 16 bit unsigned int
// 0-5 bit : to square (square 0 to 63)
// 6-11 bit : from square (square 0 to 63)
// 12-13 bit : promotion piece type (shifted by KNIGHT which is the lowest promotion to fit) or
// castle type 14-15: promotion (1), en passant (2), castling (3)
class move_t
{
  public:
    move_t() : m_data(0) {}
    constexpr explicit move_t(const std::uint16_t d) : m_data(d) {}

    constexpr move_t(const square_t from, const square_t to) : m_data((index(from) << 6) + index(to)) {}

    // to build a move if you already know the type of move
    template <move_type_t T>
    static constexpr move_t make(const square_t from, const square_t to, const piece_type_t pt = KNIGHT)
    {
        assert(T != CASTLING);
        return move_t{static_cast<uint16_t>(T + (index(pt - KNIGHT) << 12) + (index(from) << 6) + index(to))};
    }

    template <move_type_t T>
    static constexpr move_t make(const square_t from, const square_t to, const castling_type_t c)
    {
        assert(T == CASTLING);
        return move_t(static_cast<uint16_t>(T + (index(c) << 12) + (index(from) << 6) + index(to)));
    }

    // for sanity check
    [[nodiscard]] constexpr bool is_ok() const { return none().m_data != m_data && null().m_data != m_data; }

    // for these two moves from and to are the same
    static constexpr move_t null() { return move_t(65); }
    static constexpr move_t none() { return move_t(0); }

    constexpr bool operator==(const move_t& m) const { return m_data == m.m_data; }
    constexpr bool operator!=(const move_t& m) const { return m_data != m.m_data; }

    constexpr explicit operator bool() const { return m_data != 0; }

    [[nodiscard]] constexpr std::uint16_t raw() const { return m_data; }

    [[nodiscard]] constexpr square_t from_sq() const
    {
        assert(is_ok());
        return square_t{(m_data >> 6) & 0x3F};
    }

    [[nodiscard]] constexpr square_t to_sq() const
    {
        assert(is_ok());
        return square_t{m_data & 0x3F};
    }

    [[nodiscard]] constexpr move_type_t type_of() const { return static_cast<move_type_t>(m_data & 0b11 << 14); }

    [[nodiscard]] constexpr piece_type_t promotion_type() const { return piece_type_t{(m_data >> 12 & 0b11) + KNIGHT}; }

    [[nodiscard]] constexpr castling_type_t castling_type() const
    {
        return static_cast<castling_type_t>(m_data >> 12 & 0b11);
    }

    [[nodiscard]] std::string to_string() const
    {
        std::ostringstream ss;
        ss << from_sq() << to_sq();
        if (type_of() == PROMOTION)
        {
            ss << promotion_type();
        }
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const move_t mv) { return os << mv.to_string(); }

  private:
    std::uint16_t m_data;
};

struct castling_rights_t
{
    using mask_type = uint8_t;

    constexpr castling_rights_t() = default;
    constexpr explicit castling_rights_t(mask_type m) : m_mask(m) {}

    constexpr castling_rights_t(const std::initializer_list<castling_type_t> types)
    {
        for (auto t : types)
            m_mask |= t.mask();
    }

    [[nodiscard]] constexpr bool has(const castling_type_t t) const { return m_mask & t.mask(); }

    constexpr void add(const castling_type_t t) { m_mask |= t.mask(); }

    constexpr void remove(const castling_type_t t) { m_mask &= ~t.mask(); }

    constexpr void remove(const castling_rights_t other) { m_mask &= ~other.mask(); }

    constexpr void keep(const castling_rights_t other) { m_mask &= other.mask(); }

    [[nodiscard]] constexpr bool empty() const { return m_mask == 0; }

    [[nodiscard]] constexpr mask_type mask() const { return m_mask; }

    [[nodiscard]] constexpr castling_rights_t lost_from_move(const move_t move) const
    {
        castling_rights_t cur{*this};
        for (const auto square : {move.from_sq(), move.to_sq()})
            switch (index(square))
            {
                case index(E1):
                        cur.keep(castling_rights_t{WHITE_KINGSIDE, WHITE_QUEENSIDE}); return cur;
                case index(H1):
                        cur.keep(castling_rights_t{WHITE_KINGSIDE}); return cur;;
                case index(A1):
                        cur.keep(castling_rights_t{WHITE_QUEENSIDE}); return cur;;
                case index(E8):
                        cur.keep(castling_rights_t{BLACK_KINGSIDE, BLACK_QUEENSIDE}); return cur;
                case index(H8):
                        cur.keep( castling_rights_t{BLACK_KINGSIDE}); return cur;
                case index(A8):
                        cur.keep( castling_rights_t{BLACK_QUEENSIDE}); return cur;
                default: break;
            }
        return castling_rights_t{};
    }

    mask_type m_mask = 0;
};

constexpr castling_rights_t CASTLING_NONE{0};
constexpr castling_rights_t CASTLING_K{WHITE_KINGSIDE};
constexpr castling_rights_t CASTLING_Q{WHITE_QUEENSIDE};
constexpr castling_rights_t CASTLING_k{BLACK_KINGSIDE};
constexpr castling_rights_t CASTLING_q{BLACK_QUEENSIDE};

constexpr castling_rights_t CASTLING_KQ{WHITE_KINGSIDE, WHITE_QUEENSIDE};
constexpr castling_rights_t CASTLING_Kk{WHITE_KINGSIDE, BLACK_KINGSIDE};
constexpr castling_rights_t CASTLING_Kq{WHITE_KINGSIDE, BLACK_QUEENSIDE};
constexpr castling_rights_t CASTLING_Qk{WHITE_QUEENSIDE, BLACK_KINGSIDE};
constexpr castling_rights_t CASTLING_Qq{WHITE_QUEENSIDE, BLACK_QUEENSIDE};
constexpr castling_rights_t CASTLING_kq{BLACK_KINGSIDE, BLACK_QUEENSIDE};

constexpr castling_rights_t CASTLING_KQk{WHITE_KINGSIDE, WHITE_QUEENSIDE, BLACK_KINGSIDE};
constexpr castling_rights_t CASTLING_KQq{WHITE_KINGSIDE, WHITE_QUEENSIDE, BLACK_QUEENSIDE};
constexpr castling_rights_t CASTLING_Kkq{WHITE_KINGSIDE, BLACK_KINGSIDE, BLACK_QUEENSIDE};
constexpr castling_rights_t CASTLING_Qkq{WHITE_QUEENSIDE, BLACK_KINGSIDE, BLACK_QUEENSIDE};

constexpr castling_rights_t CASTLING_KQkq{WHITE_KINGSIDE, WHITE_QUEENSIDE, BLACK_KINGSIDE, BLACK_QUEENSIDE};

template <>
struct enum_utils<castling_rights_t>
{
    using enum_type       = castling_rights_t;
    using idx_type        = uint8_t;
    using underlying_type = castling_rights_t::mask_type;

    static constexpr idx_type count           = 16;
    static constexpr bool     is_well_ordered = true;

    static constexpr repr_type<enum_type> repr =  {
        "-", "K", "Q", "KQ", "k", "Kk", "Qk", "KQk",
        "q", "Kq", "Qq", "KQq","kq", "Kkq", "Qkq", "KQkq"
    };

    static constexpr underlying_type value_of(const castling_rights_t cr) { return cr.m_mask; }
};


namespace bit
{
    template <std::unsigned_integral T>
    constexpr int popcount(const T x) noexcept
    {
        return std::popcount(x);
    }

    template <std::unsigned_integral T>
    constexpr int get_lsb(const T bb) noexcept
    {
        return std::countr_zero(bb);
    }

    template <std::unsigned_integral T>
    constexpr int get_msb(const T bb) noexcept
    {
        return std::countl_zero(bb);
    }

    template <std::unsigned_integral T>
    constexpr int pop_lsb(T& bb) noexcept
    {
        int n = std::countr_zero(bb);
        bb &= ~(static_cast<T>(1) << n);
        return n;
    }

    template <typename T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
    constexpr T shift_left(const T value, const unsigned shift)
    {
        // assert(shift < sizeof(T) * 8 && "shift exceeds bit width");
        return value << shift;
    }

    template <typename T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
    constexpr T shift_right(const T value, const unsigned shift)
    {
        // assert(shift < sizeof(T) * 8 && "shift exceeds bit width");
        return value >> shift;
    }

} // namespace bit

constexpr size_t MAX_PLY = 255;

constexpr int MATE_SCORE = 100000;
constexpr int INFINITE   = 1000000;

#endif
