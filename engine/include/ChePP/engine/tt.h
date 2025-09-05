//
// Created by paul on 7/30/25.
//

#ifndef TT_H
#define TT_H

#include "types.h"

#include <bits/shared_ptr_base.h>
#include <optional>
#include <vector>
#include "ChePP/engine/zobrist.h"

enum tt_bound_t : uint8_t {
    EXACT,
    LOWER,
    UPPER,
};

struct tt_entry_t
{

    tt_entry_t() noexcept = default;
    tt_entry_t(const hash_t hash, const int depth, const int score, const tt_bound_t bound, const int generation, const Move move)
        : m_hash(hash), m_depth(depth), m_score(score), m_move(move), m_bound(bound), m_generation(generation)
    {
    }

    hash_t  m_hash;
    uint16_t m_depth;
    int16_t m_score;
    Move m_move;
    tt_bound_t m_bound;
    uint8_t m_generation;
};


inline uint64_t floor_power_of_two(const uint64_t x) {
    if (x == 0) return 0;
    return 1ULL << (63 - __builtin_clzll(x));
}

struct tt_t
{

    void init (const size_t mb)
    {
        m_size = floor_power_of_two(mb * 1024 * 1024 / sizeof(tt_entry_t));
        std::cout << m_size << std::endl;
        m_table.resize(m_size);
        std::ranges::fill(m_table, tt_entry_t());
    }

    void prefetch(hash_t hash) const noexcept {
        const size_t idx = index(hash);
        __builtin_prefetch(&m_table[idx], 0, 3);
    }

    [[nodiscard]] std::optional<tt_entry_t> probe(const hash_t hash, const int depth, const int alpha, const int beta) const
    {
        const tt_entry_t& cur = m_table[index(hash)];
        if (cur.m_hash != hash)
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

    void store(const hash_t hash, const int depth, const int score, const int alpha, const int beta, const Move move)
    {
        const tt_entry_t& cur = m_table[index(hash)];
        tt_bound_t bound;
        if (score <= alpha) {
            bound = UPPER;
        } else if (score >= beta) {
            bound = LOWER;
        } else {
            bound = EXACT;
        }
        const auto  entry = tt_entry_t(hash , depth, score, bound, m_generation, move);
        if (bool replace = cur.m_depth <= depth || cur.m_generation != m_generation; !replace) return;
        if (cur.m_bound == EXACT || cur.m_hash != hash|| depth + 4 > cur.m_depth) {
            //if (cur.m_bound == EXACT) return;
            m_table[index(hash)] = entry;
        }
    }

    void new_generation()
    {
        m_generation++;
    }

private:
    [[nodiscard]] size_t index(const hash_t hash) const
    {
        return hash & (m_size - 1);
    }

    int m_generation = 0;
    std::size_t m_size = 0;
    std::vector<tt_entry_t> m_table;
};

inline tt_t g_tt;



#endif //TT_H
