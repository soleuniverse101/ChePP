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
    tt_entry_t(const hash_t hash, const int depth, const int score) : m_hash(hash), m_depth(depth), m_score(score) {}
    hash_t  m_hash;
    int m_depth;
    int m_score;
};


inline uint64_t floor_power_of_two(uint64_t x) {
    if (x == 0) return 0;
    return 1ULL << (63 - __builtin_clzll(x));
}

struct tt_t
{

    void init (const size_t mb)
    {
        m_size = floor_power_of_two(mb * 1024 * 1024);
        m_table = std::make_unique<tt_entry_t[]>(m_size);
    }

    std::optional<tt_entry_t> probe(const hash_t hash, const int depth) const
    {
        const tt_entry_t& cur = m_table[index(hash)];
        if (cur.m_hash == hash && cur.m_depth >= depth)
        {
            return cur;
        }
        return std::nullopt;
    }

    void store(const tt_entry_t entry)
    {
        tt_entry_t& cur = m_table[index(entry.m_hash)];
        if (cur.m_hash != entry.m_hash || cur.m_depth < entry.m_depth)
        {
             cur = entry;
        }
    }

private:
    size_t index(const hash_t hash) const
    {
        return hash & (m_size - 1);
    }

    std::size_t m_size = 0;
    std::unique_ptr<tt_entry_t[]> m_table;
};

extern tt_t g_tt;



#endif //TT_H
