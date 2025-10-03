#include <iostream>
#include "cpu_features.h"
#include "../operations.h"


int main() {
    using D8 = simd::pick_tag_t<int8_t, 256>;
    using D16 = simd::pick_tag_t<int8_t, 256>;
    using D32 = simd::pick_tag_t<int32_t, 256>;
    using vecother = simd::register_type_t<simd::pick_tag_t<int32_t, 512>>;
    using vec8 = simd::register_type_t<D8>;
    using vec32 = simd::register_type_t<D32>;
    alignas(64) int8_t underlying[64] {1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4, 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4, 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4, 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4};
    vec8 a = *reinterpret_cast<vec8 *>(underlying);
    vec8 b = *reinterpret_cast<vec8 *>(underlying);
    vecother d = *reinterpret_cast<vecother *>(underlying);
    vec32 c = simd::set1<D32>(0);
    std::cout << simd::operation_supported<simd::add<D16>, decltype(a), decltype(d)> << std::endl;
    auto test2 = SIMD_CALL(simd::add<D16>, simd::register_type_t<D16>, a, d);
    constexpr auto test = simd::operation_supported<simd::store<D8>, int8_t*, vec8>;
    for (int i = 0; i <10; i++)
        c = simd::call_or_default<simd::mul_quad_add<D8>, simd::register_type_t<D32>>(a, b, c);
    std::cout << +*reinterpret_cast<int32_t*>(&c) << "\n";
    return 0;
}