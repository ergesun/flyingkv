/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_TOKEN_BUCKET_LIMITER_H
#define FLYINGKV_ACC_TOKEN_BUCKET_LIMITER_H

#include "../../sys/spin-lock.h"
#include "../../common/common-def.h"

#include "common-def.h"
#include "limiter.h"

namespace flyingkv {
namespace acc {
class TokenBucketLimiter : public ILimiter, public IConfParser {
public:
    explicit TokenBucketLimiter(int64_t timeUnit = NANOSEC);
    TokenBucketLimiter(int64_t capacity, uint32_t speed, int64_t timeUnit);
    ~TokenBucketLimiter() override;

    bool GrantUntil(int64_t deadlineTs, common::ReqRespType type) override;
    void GiveBack(common::ReqRespType rt) override;
    bool Parse(cJSON *blockRoot) override;
    std::string GetName() override {
        return m_sName;
    }

private:
    bool update_and_consume_tokens(int64_t currentTs, int64_t requiredTokenCnt);
    bool consume_tokens(int64_t requiredTokenCnt);
    void initialize();
    void update(int64_t currentTs);
    int64_t now_timestamps();
    int64_t cal_wait_time(int64_t tokens);
    int64_t calc_tokens_to_add(int64_t passedTime);
    void put_tokens(int64_t tokens);
    inline bool is_full() const {
        return m_currentResCount >= m_maxResCnt;
    }

private:
    uint32_t          m_speed;
    spin_lock_t       m_sl = UNLOCKED;
    int64_t           m_lastUpdateTs; // last updated timestamp
    const int64_t     m_timeUnit; // Time granularity for one second
    int64_t           m_tokenPerTimeUnit;
    int64_t           m_timeRemainder;
    int64_t           m_timeAccumulated;
    LimiterType       type;
    RWT              *m_pRWT = nullptr;
    int64_t           m_maxResCnt;  // Max resource value
    int64_t           m_currentResCount;
    std::string       m_sName;
    bool              m_bSkipped = false;
#ifdef UNIT_TEST // We expose these functions for unit test only
public:
#endif

};
} // namespace acc
} // namespace flyingkv

#endif //FLYINGKV_ACC_TOKEN_BUCKET_LIMITER_H
