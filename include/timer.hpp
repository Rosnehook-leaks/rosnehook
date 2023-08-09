/*
 * timer.hpp
 *
 *  Created on: Jul 29, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <chrono>

class Timer
{
public:
    std::chrono::time_point<std::chrono::system_clock> last{};

    constexpr Timer() noexcept = default;

    inline bool check(unsigned int ms) const noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last).count() >= ms;
    }

    bool test_and_set(unsigned int ms) noexcept
    {
        if (check(ms))
        {
            update();
            return true;
        }
        return false;
    }

    inline void update() noexcept
    {
        last = std::chrono::system_clock::now();
    }
};
