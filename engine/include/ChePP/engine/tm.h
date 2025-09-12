#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>

struct TimeParams {
    int wtime_ms = -1;
    int btime_ms = -1;
    int winc_ms  = -1;
    int binc_ms  = -1;
    int movestogo = -1;
    int movetime = -1;
    int depth = -1;
};

struct TimeManager {

    TimeManager() = default;
    explicit TimeManager(const TimeParams& params, const Color side) : m_params(params), m_side(side) {}

    void start()
    {
        compute_max_time();
        m_start_time = std::chrono::steady_clock::now();
        m_stop_flag = false;
    }

    int max_time_ms() const { return m_max_time_ms; }

    bool should_stop() {
        return m_stop_flag;
    }

    void update(int depth = -1)
    {
        if (m_params.depth > 0 && depth > 0 && depth > m_params.depth) {
            m_stop_flag = true;
            return;
        }
        if (max_time_ms() > 0)
        {
            auto elapsed = std::chrono::steady_clock::now() - m_start_time;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= m_max_time_ms) {
                m_stop_flag = true;
                return;
            }
        }

    }

    void stop() { m_stop_flag = true; }

    void compute_max_time() {
        if (m_params.movetime > 0)
        {
            m_max_time_ms = m_params.movetime;
            return;
        }
        int time_left = m_side == WHITE ? m_params.wtime_ms : m_params.btime_ms;
        int inc = m_side == BLACK ? m_params.winc_ms : m_params.binc_ms;

        if (time_left <= 0) {
            m_max_time_ms = -1;
            return;
        }


        int moves_to_go = m_params.movestogo > 0 ? m_params.movestogo : 30;

        m_max_time_ms = std::max(1, (time_left / moves_to_go) + inc);

        m_max_time_ms = std::max(50, m_max_time_ms - 50);

        if (m_max_time_ms > 60 * 1000) m_max_time_ms = 60 * 1000;
    }

private:
    TimeParams m_params{};
    Color m_side{};

    std::chrono::steady_clock::time_point m_start_time{};

    int m_max_time_ms{-1};
    bool m_stop_flag{false};
};

#endif // TIME_MANAGER_H
