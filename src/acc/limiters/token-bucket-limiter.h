/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_TOKEN_BUCKET_LIMITER_H
#define FLYINGKV_ACC_TOKEN_BUCKET_LIMITER_H

#include "../../sys/spin-lock.h"
#include "../../common/common-def.h"

#include "../igranter.h"

#include "../common-def.h"
#include "../../3rd/cjson/cJSON.h"
#include "../config.h"

namespace flyingkv {
namespace acc {
class RWT;
class TokenBucketLimiter : public IGranter {
PUBLIC
    explicit TokenBucketLimiter(int64_t timeUnit = NANOSEC);
    TokenBucketLimiter(int64_t capacity, uint32_t speed, int64_t timeUnit);
    ~TokenBucketLimiter() override;

    void Init(const TokenBucketLimiterConfig *conf);
    bool GrantUntil(int64_t deadlineTs, common::ReqRespType type) override;
    void GiveBack(common::ReqRespType rt) override;
    bool Parse(cJSON *blockRoot);
    std::string GetName() override {
        return m_sName;
    }

PRIVATE
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

PRIVATE
    spin_lock_t       m_sl = UNLOCKED;
    int64_t           m_lastUpdateTs; // last updated timestamp
    int64_t           m_tokenPerTimeUnit;
    int64_t           m_timeRemainder;
    int64_t           m_timeAccumulated;
    LimiterType       type;
    RWT              *m_pRWT = nullptr;
    int64_t           m_maxResCnt;  // Max resource value
    int64_t           m_currentResCount;
    int64_t           m_speed;
    const int64_t     m_timeUnit; // Time granularity for one second
    std::string       m_sName;
    bool              m_bSkipped = false;
};
} // namespace acc
} // namespace flyingkv

#endif //FLYINGKV_ACC_TOKEN_BUCKET_LIMITER_H
