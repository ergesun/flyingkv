/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_AC_RLC_H
#define MINIKV_AC_RLC_H

#include "../../common/ikv-common.h"

namespace minikv {
namespace acc {
class ILimiter {
public:
    virtual ~ILimiter() = default;

    virtual bool GrantUntil(int64_t deadlineTs, common::ReqRespType) = 0;
    virtual void GiveBack(common::ReqRespType) = 0;
    virtual std::string GetName() = 0;
};
}
}

#endif //MINIKV_AC_RLC_H

