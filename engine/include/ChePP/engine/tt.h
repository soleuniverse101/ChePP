//
// Created by paul on 7/30/25.
//

#ifndef TT_H
#define TT_H

#include "types.h"

#include <bits/shared_ptr_base.h>
#include <optional>

enum tt_bound_t : uint8_t {
    EXACT,
    LOWER,
    UPPER,
};

struct tt_entry_t
{

    tt_entry_t() noexcept = default;
    tt_entry_t(const hash_t hash, const int depth, const int score, const tt_bound_t bound, const int generation)
        : m_hash(hash), m_depth(depth), m_score(score), m_bound(bound), m_generation(generation)
    {
    }

    hash_t  m_hash;
    int m_depth;
    int m_score;
    tt_bound_t m_bound;

    int m_generation;
};


inline uint64_t floor_power_of_two(const uint64_t x) {
    if (x == 0) return 0;
    return 1ULL << (63 - __builtin_clzll(x));
}

struct tt_t
{

    void init (const size_t mb)
    {
        m_size = floor_power_of_two(mb * 1024 * 1024);
        m_generation = 0;
        m_table = std::make_unique<tt_entry_t[]>(m_size);
    }

    [[nodiscard]] std::optional<tt_entry_t> probe(const hash_t hash, const int depth, const int alpha, const int beta) const
    {
        const tt_entry_t& cur = m_table[index(hash)];
        if (cur.m_hash != hash || cur.m_depth < depth || cur.m_generation != m_generation)
        {
            return std::nullopt;
        }
        switch (cur.m_bound) {
            case EXACT:
                return cur;
            case LOWER:
                if (cur.m_score >= beta)
                    return cur;
                break;
            case UPPER:
                if (cur.m_score <= alpha)
                    return cur;
                break;
        }
    return std::nullopt;
    }

    void store(const hash_t hash, const int depth, const int score, const int alpha, const int beta)
    {
        tt_entry_t& cur = m_table[index(hash)];
        tt_bound_t bound;
        if (score <= alpha) {
            bound = UPPER;
        } else if (score >= beta) {
            bound = LOWER;
        } else {
            bound = EXACT;
        }
        const auto  entry = tt_entry_t(hash, depth, score, bound, m_generation);
        if (cur.m_generation != m_generation || (cur.m_hash != hash && cur.m_depth < depth)) {
            m_table[index(hash)] = entry;
        }
    }

    void new_generation()
    {
        m_generation++;
    }

private:
    size_t index(const hash_t hash) const
    {
        return hash & (m_size - 1);
    }

    int m_generation = 0;
    std::size_t m_size = 0;
    std::unique_ptr<tt_entry_t[]> m_table;
};

extern tt_t g_tt;



#endif //TT_H
