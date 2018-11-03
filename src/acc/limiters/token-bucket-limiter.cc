/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <mutex>

#include "../../sys/cctime.h"
#include "../../common/common-def.h"

#include "../rwt/rwt.h"

#include "token-bucket-limiter.h"

using flyingkv::sys::SpinLock;

namespace flyingkv {
namespace acc {
TokenBucketLimiter::TokenBucketLimiter(int64_t timeUnit) : m_timeUnit(timeUnit){
    type = LimiterType::TOKEN_BUCKET;
}

TokenBucketLimiter::TokenBucketLimiter(int64_t capacity, uint32_t speed, int64_t timeUnit)
                  : m_maxResCnt(capacity), m_speed(speed), m_timeUnit(timeUnit){
    initialize();
}

TokenBucketLimiter::~TokenBucketLimiter() {
    DELETE_PTR(m_pRWT);
}

void TokenBucketLimiter::Init(const TokenBucketLimiterConfig *conf) {
    m_sName = conf->Name;
    m_bSkipped = conf->Skip;
    m_maxResCnt = conf->MaxSize;
    m_speed = conf->Speed;
    m_pRWT = new RWT();
    m_pRWT->Init(&conf->Rwt);
}

bool TokenBucketLimiter::GrantUntil(int64_t deadlineTs, common::ReqRespType type) {
        if (m_bSkipped) {
            return true;
        }

        if (deadlineTs >= now_timestamps()) {
            return false;
        }

        auto weight = m_pRWT->GetWeight(type);
        if (0 == weight) {
            return true;
        }

        if (weight > m_maxResCnt) {
            LOGFFUN << "req " << int(type) << "rejected because need tokens greater than token-bucket max resources count.";
            return false;
        }

        bool ok;
        {
            SpinLock l(&m_sl);
            auto now = now_timestamps();
            ok = update_and_consume_tokens(now, weight);
        }

        if (ok) {
            LOGDFUN << "req " << int(type) << " passed by " << m_sName;
            return true;
        }

        while (true) {
            auto nowTs = now_timestamps();
            if (deadlineTs >= nowTs) {
                return false;
            }

            auto waitTokens = weight - m_currentResCount;
            auto waitTime = cal_wait_time(waitTokens);
            if (deadlineTs + waitTime >= nowTs) {
                return false;
            }
            timespec ts = {
                    .tv_sec = waitTime / NANOSEC,
                    .tv_nsec = waitTime % NANOSEC
            };
            timespec leftTs = {.tv_sec = 0, .tv_nsec = 0};
            nanosleep(&ts, &leftTs);
            {
                SpinLock l(&m_sl);
                auto now = now_timestamps();
                ok = update_and_consume_tokens(now, weight);
            }

            if (ok) {
                return true;
            }
        }
}

void TokenBucketLimiter::GiveBack(const common::ReqRespType rt) {
    if (m_bSkipped) {
        return;
    }

    auto weight = m_pRWT->GetWeight(rt);
    if (0 == weight) {
        return;
    }

    SpinLock l(&m_sl);
    put_tokens(weight);
}

// Initialize the private mem variables
void TokenBucketLimiter::initialize(){
    m_lastUpdateTs = now_timestamps();
    m_tokenPerTimeUnit = m_speed / m_timeUnit;
    m_timeRemainder = m_speed % m_timeUnit;
    m_timeAccumulated = 0;
    m_currentResCount = 0;
}

// Update current token bucket status
void TokenBucketLimiter::update(int64_t currentTs){
    auto passedTime = currentTs - m_lastUpdateTs;
    // 2 simple optimizations before update work
    if (0 == passedTime) {
        return;
    }

    if (is_full()) {
        m_lastUpdateTs = currentTs;
        return;
    }

        put_tokens(calc_tokens_to_add(passedTime));
    m_lastUpdateTs = currentTs;
}

int64_t TokenBucketLimiter::calc_tokens_to_add(int64_t passedTime) {
    auto tokensToAdd = m_tokenPerTimeUnit * passedTime;
    m_timeAccumulated += m_timeRemainder * passedTime;
    if (m_timeAccumulated >= m_timeUnit) {
        tokensToAdd += m_timeAccumulated / m_timeUnit;
        m_timeAccumulated %= m_timeUnit;
    }
    return tokensToAdd;
}

int64_t TokenBucketLimiter::now_timestamps() {
    return sys::cctime::GetCurrentTime().get_total_nsecs();
}

// Get tokens from the bucket
// The tokens should be taken all or nothing
// If succeed return token num request or return 0
bool TokenBucketLimiter::update_and_consume_tokens(int64_t currentTs, int64_t requiredTokenCnt) {
    update(currentTs);
    return consume_tokens(requiredTokenCnt);
}

bool TokenBucketLimiter::consume_tokens(int64_t requiredTokenCnt) {
    if (requiredTokenCnt <= m_currentResCount) {
        m_currentResCount -= requiredTokenCnt;
        return true;
    }

    return false;
}

int64_t TokenBucketLimiter::cal_wait_time(int64_t tokens) {
    auto ticks = tokens * m_timeUnit;
    auto t = ticks / m_speed;
    ticks %= m_speed;
    if (0 != ticks) {
        ++t;
    }

    return t * (NANOSEC / m_timeUnit);
}

// Put tokens in bucket
void TokenBucketLimiter::put_tokens(int64_t tokens) {
    m_currentResCount += tokens;
    // Drop the overflow bucket
    if (is_full()) {
       m_currentResCount = m_maxResCnt;
    }
}
} // namespace acc
} // namespace flyingkv
