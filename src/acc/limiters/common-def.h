/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_ACC_LIMITER_COMMON_DEF_H
#define MINIKV_ACC_LIMITER_COMMON_DEF_H

namespace minikv {
namespace acc {
enum class LimiterType {
    REF_COUNTER   = 0,
    TOKEN_BUCKET
};
}
}

#endif //MINIKV_ACC_LIMITER_COMMON_DEF_H
