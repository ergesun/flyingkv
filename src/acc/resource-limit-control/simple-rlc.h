/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_SIMPLE_RLC_H
#define FLYINGKV_ACC_SIMPLE_RLC_H

#include "../igranter.h"
#include "../config.h"
#include "../../3rd/cjson/cJSON.h"

namespace flyingkv {
namespace acc {
class SimpleRlc : public IGranter {
PUBLIC
    SimpleRlc() = default;
    ~SimpleRlc() override;

    bool Init(const RlcConfig *conf);
    bool GrantUntil(int64_t deadlineTs, common::ReqRespType type) override;
    void GiveBack(common::ReqRespType rt) override;
    std::string GetName() override {
        return "simple rlc";
    }

PRIVATE
    bool contains_granter(IGranter *granter) const;

PRIVATE
    bool                    m_bSkipped     = false;
    std::vector<IGranter*>  m_vGranters;
    uint8_t                 m_iGrantersCnt = 0;
};
}
}

#endif //FLYINGKV_ACC_SIMPLE_RLC_H
