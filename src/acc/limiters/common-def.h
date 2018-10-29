/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_LIMITER_COMMON_DEF_H
#define FLYINGKV_ACC_LIMITER_COMMON_DEF_H

namespace flyingkv {
namespace acc {
enum class LimiterType {
    REF_COUNTER   = 0,
    TOKEN_BUCKET
};
}
}

#endif //FLYINGKV_ACC_LIMITER_COMMON_DEF_H
