/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../common-def.h"
#include "../limiters/token-bucket-limiter.h"

#include "simple-rlc.h"

namespace flyingkv {
namespace acc {
SimpleRlc::~SimpleRlc() {
    if (0 != m_iGrantersCnt) {
        for (auto g : m_vGranters) {
            DELETE_PTR(g);
        }

        m_vGranters.clear();
    }
}

bool SimpleRlc::Init(const RlcConfig *conf) {
    auto rc = static_cast<SimpleRlcConfig*>(conf);
    m_bSkipped = rc->Skip;
    for (auto &lc : rc->Limiters) {
        IGranter *pg = nullptr;
        switch (lc->Type) {
            case LimiterType::REF_COUNTER:
                break;
            case LimiterType::TOKEN_BUCKET:
                auto tb = new TokenBucketLimiter();
                tb->Init(static_cast<TokenBucketLimiterConfig*>(&lc));
                pg = tb;
        }

        if (!pg) {
            return false;
        }

        if (contains_granter(pg)) {
            throw std::runtime_error("Cannot define repetitive limiter = " + pg->GetName());
        }

        m_vGranters.push_back(pg);
    }

    m_iGrantersCnt = (uint8_t)m_vGranters.size();
    return true;
}

bool SimpleRlc::contains_granter(IGranter *granter) const {
    for (auto &vg : m_vGranters) {
        if (vg->GetName() == granter->GetName()) {
            return true;
        }
    }

    return false;
}

bool SimpleRlc::GrantUntil(int64_t deadline, common::ReqRespType rt) {
    if (m_bSkipped) {
        return true;
    }

    if (0 == m_iGrantersCnt) {
        return true;
    }

    auto notOKIdx = -1;
    for (int i = 0; i < m_iGrantersCnt; ++i) {
        if (!m_vGranters[i]->GrantUntil(deadline, rt)) {
            notOKIdx = i;
            break;
        }
    }

    if (-1 != notOKIdx) {
        for (int i = 0; i < notOKIdx; ++i) {
            m_vGranters[i]->GiveBack(rt);
        }

        LOGWFUN <<"Req " << int(rt) <<" grant rejected by ACC";
        return false;
    }

    LOGDFUN3("Request ", int(rt), " grant passed by ACC.");
    return true;
}

void SimpleRlc::GiveBack(common::ReqRespType rt) {
    // nothing to do.
}
}
};

