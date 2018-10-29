/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_SYS_RANDOM_H
#define FLYINGKV_SYS_RANDOM_H

#include <sys/time.h>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <cassert>

namespace flyingkv {
namespace sys {
class Random {
PUBLIC
    struct Range {
        Range() {
            start = end = INVALID_RANGE;
        }
        Range(int32_t s, int32_t e) throw(std::invalid_argument) : start(s), end(e) {
            assert(s < e && s > 0 && e > 0);
        }

    PRIVATE
        friend class Random;
        int32_t start, end;
        static const int32_t INVALID_RANGE;
    };

PUBLIC
    Random() {
        init_ts_seed();
    }

    explicit Random(uint32_t seed) {
        srand(seed);
        m_r.start = m_r.end = Range::INVALID_RANGE;
    }
    explicit Random(Range r) : m_r(r) {
        init_ts_seed();
    }
    Random(uint32_t seed, Range r) : m_r(r) {
        srand(seed);
    }

    int32_t GetNew();
    static int32_t GetNewWithSRand();

PRIVATE
    static inline void init_ts_seed() {
        timeval tv;
        gettimeofday(&tv, nullptr);
        srand(static_cast<uint32_t >(tv.tv_sec * 1000 + tv.tv_usec / 1000));
    }

    inline bool check_range() {
        return !(Range::INVALID_RANGE == m_r.start || Range::INVALID_RANGE == m_r.end);
    }

PRIVATE
    Range m_r;
};
}
}

#endif //FLYINGKV_SYS_RANDOM_H
