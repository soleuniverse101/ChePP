//
// Created by paul on 7/30/25.
//

#ifndef TT_H
#define TT_H

#include "types.h"

#include <bits/shared_ptr_base.h>
#include <optional>

struct tt_entry_t
{

    tt_entry_t() noexcept = default;
    tt_entry_t(const hash_t hash, const int depth, const int score, const int generation)
        : m_hash(hash), m_depth(depth), m_score(score), m_generation(generation)
    {
    }
    [[nodiscard]] int ply() const
    {
        return m_generation + m_depth;
    }
    hash_t  m_hash;
    int m_depth;
    int m_score;

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

    std::optional<tt_entry_t> probe(const hash_t hash, const int depth) const
    {
        const tt_entry_t& cur = m_table[index(hash)];
        if (cur.m_hash == hash && cur.ply() >= ply(depth))
        {
            return cur;
        }
        return std::nullopt;
    }

    void store(const hash_t hash, const int depth, const int score)
    {
        const auto  entry = tt_entry_t(hash, depth, score, m_generation);
        tt_entry_t& cur = m_table[index(entry.m_hash)];
        if (cur.m_hash != entry.m_hash || cur.ply() < entry.ply())
        {
            cur = entry;
        }
    }

    void new_generation()
    {
        m_generation++;
    }

private:
    int ply(const int depth) const
    {
        return m_generation + depth;
    }
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
