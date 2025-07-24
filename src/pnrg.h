#ifndef PNRG_H
#define PNRG_H

#include "types.h"


// XOR shift algo, need to check exactly what it does, just use for now
// http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
class PRNG {

    uint64_t s;

    uint64_t rand64() {

        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }

public:
    explicit PRNG(const uint64_t seed) :
        s(seed) {
        assert(seed);
    }

    template<typename T>
    T rand() {
        return T(rand64());
    }

    // Special generator used to fast init magic numbers.
    // Output values only have 1/8th of their bits set on average.
    template<typename T>
    T sparse_rand() {
        return T(rand64() & rand64() & rand64());
    }
};


#endif PNRG_H
