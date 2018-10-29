/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_AC_RLC_H
#define FLYINGKV_AC_RLC_H

#include "../../common/ikv-common.h"

namespace flyingkv {
namespace acc {
class IRlc {
public:
    virtual ~IRlc() = default;

    virtual bool CanGrant(common::ReqRespType) = 0;
    virtual int64_t EstimateGrantWaitTime(common::ReqRespType) = 0;
    virtual bool GrantWhen(common::ReqRespType) = 0;
    virtual bool GrantUntil(uint32_t ms, common::ReqRespType) = 0;
};
}
}

#endif //FLYINGKV_AC_RLC_H