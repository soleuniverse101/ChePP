#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "cpu_features.h"

#if __CHEPP_SSE2__ || __CHEPP_SSE3__
#define __CHEPP_SIMD_X86__
#include <emmintrin.h>
#endif
#if __CHEPP_AVX2__ || __CHEPP_AVX512__
#define __CHEPP_SIMD_X86__
#include <immintrin.h>
#endif
#if __CHEPP_NEON__
#define __CHEPP_SIMD_NEON__
#include <arm_neon.h>
#endif

namespace simd {
    struct ArchTag {
    };

    struct Scalar : ArchTag {
    };

    struct X86 : ArchTag {
    };

    struct SSE2 : X86 {
    };

    struct SSE3 : X86 {
    };

    struct AVX2 : X86 {
    };

    struct AVX512 : X86 {
    };

    struct Neon : ArchTag {
    };

    struct Neon32 : Neon {
    };

    struct Neon64 : Neon {
    };

    using selected_arch =
#if __CHEPP_AVX512__
    AVX512;
#elif __CHEPP_AVX2__
    AVX2;
#elif __CHEPP_SIMD_SSE3__
        SSE3;
#elif __CHEPP_SIMD_SSE2__
    SSE2;
#elif __CHEPP_NEON_V8__
    Neon64;
#elif __CHEPP_NEON__
    Neon32;
#else
    ScalarArch;
#endif


    template<typename T, typename Arch, std::size_t Bits>
    struct VecTag {
        using value_type = T;
        using arch = Arch;
        static constexpr std::size_t width_bits = Bits;
    };

    template<typename T>
    using VecScalar = VecTag<T, Scalar, sizeof(T)>;
    template<typename T, typename Arch>
    using Vec128 = VecTag<T, Arch, 128>;
    template<typename T, typename Arch>
    using Vec256 = VecTag<T, Arch, 256>;
    template<typename T, typename Arch>
    using Vec512 = VecTag<T, Arch, 512>;


    template<typename Vec>
    struct bit_count;

    template<typename T, typename Arch, std::size_t Bits>
    struct bit_count<VecTag<T, Arch, Bits> > {
        static constexpr std::size_t value = Bits;
    };

    template<typename Vec>
    inline constexpr std::size_t bit_count_v = bit_count<Vec>::value;

    template <typename Tag>
    using arch_t = typename Tag::arch;

    template <typename Tag>
    using value_t = typename Tag::value_type;


    template<typename Vec>
    struct lane_count;

    template<typename T, typename Arch, std::size_t Bits>
    struct lane_count<VecTag<T, Arch, Bits> > {
        static constexpr std::size_t value = Bits / (8 * sizeof(T));
    };

    template<typename Vec>
    inline constexpr std::size_t lane_count_v = lane_count<Vec>::value;


    template<typename Arch>
    struct register_count;

    template<>
    struct register_count<Scalar> {
        static constexpr std::size_t value = 32;
    };

    template<>
    struct register_count<SSE2> {
        static constexpr std::size_t value = 16;
    };


    template<>
    struct register_count<SSE3> {
        static constexpr std::size_t value = 16;
    };

    template<>
    struct register_count<AVX2> {
        static constexpr std::size_t value = 16;
    };

    template<>
    struct register_count<AVX512> {
        static constexpr std::size_t value = 32;
    };

    template<>
    struct register_count<Neon32> {
        static constexpr std::size_t value = 16;
    };

    template<>
    struct register_count<Neon64> {
        static constexpr std::size_t value = 32;
    };

    template<typename Arch>
    inline constexpr std::size_t register_count_v = register_count<Arch>::value;

    template<typename Arch>
    concept is_x86_v = std::is_base_of_v<X86, Arch>;

    template<typename Arch>
    concept is_neon_v = std::is_base_of_v<Neon, Arch>;

    template<typename VecTag>
    struct register_type;

    template<typename T>
    struct register_type<VecScalar<T> > {
        using type = T;
    };

#if __CHEPP_SSE2__ || __CHEPP_SSE3__
    template<typename T, typename Arch>
    requires (std::is_same_v<Arch, SSE2> || std::is_same_v<Arch, SSE3>)
    struct register_type<Vec128<T, Arch>> {
        using type = __m128i;
    };
#endif
#if __CHEPP_AVX2__
    template<typename T>
    struct register_type<Vec128<T, AVX2> > {
        using type = __m128i;
    };

    template<typename T>
    struct register_type<Vec256<T, AVX2> > {
        using type = __m256i;
    };
#endif
#if __CHEPP_AVX512__
    template<typename T>
    struct register_type<Vec128<T, AVX512> > {
        using type = __m128i;
    };

    template<typename T>
    struct register_type<Vec256<T, AVX512> > {
        using type = __m256i;
    };

    template<typename T>
    struct register_type<Vec512<T, AVX512> > {
        using type = __m512i;
    };
#endif

#if __CHEPP_NEON__
    template<>
    struct register_type<Vec128<int8_t, Neon32> > {
        using type = int8x16_t;
    };
    template<>
    struct register_type<Vec128<int16_t, Neon32> > {
        using type = int16x8_t;
    };
    template<>
    struct register_type<Vec128<int32_t, Neon32> > {
        using type = int32x4_t;
    };

    template<>
    struct register_type<Vec128<int8_t, Neon64> > {
        using type = int8x16_t;
    };
    template<>
    struct register_type<Vec128<int16_t, Neon64> > {
        using type = int16x8_t;
    };
    template<>
    struct register_type<Vec128<int32_t, Neon64> > {
        using type = int32x4_t;
    };
#endif

    template<typename Vec>
    using register_type_t = typename register_type<Vec>::type;

    template <auto F, typename... Args>
    concept operation_supported =
    requires(Args... args) {
        { F(args...) } -> std::same_as<decltype(F(args...))>;
    }
    && !std::is_same_v<decltype(F(std::declval<Args>()...)), std::false_type>;


    template <auto Op, typename RetT, typename... CallArgs>
    __always_inline RetT
    call_or_default(CallArgs&&... args) {
        if constexpr (operation_supported<Op, std::remove_cvref_t<CallArgs>...>) {
            return Op(std::forward<CallArgs>(args)...);
        } else {
            return RetT{};
        }
    }


#define SIMD_CALL(OPNAME, RETTYPE, ...) \
simd::call_or_default<OPNAME, RETTYPE>(__VA_ARGS__)

    template<typename T, std::size_t N = 0, typename Arch = selected_arch>
    struct pick_tag {
        using type = std::conditional_t<
            is_x86_v<Arch>,
            std::conditional_t<
                N == 0,
                std::conditional_t<__CHEPP_AVX512__, Vec512<T, Arch>,
                    std::conditional_t<__CHEPP_AVX2__, Vec256<T, Arch>,
                        std::conditional_t<__CHEPP_SSE2__ || __CHEPP_SSE3__, Vec128<T, Arch>,
                            VecScalar<T> > > >,
                std::conditional_t<N <= 128, Vec128<T, Arch>,
                    std::conditional_t<N <= 256, Vec256<T, Arch>,
                        std::conditional_t<N <= 512, Vec512<T, Arch>,
                            VecScalar<T> > > >
            >,
            std::conditional_t<is_neon_v<Arch>,
                Vec128<T, Arch>,
                VecScalar<T>
            >
        >;
    };

    template<typename T, std::size_t N = 0>
    using pick_tag_t = typename pick_tag<T, N>::type;
} // namespace simd
