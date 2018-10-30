/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_RWT_H
#define FLYINGKV_ACC_RWT_H

#include <unordered_map>
#include <string>

#include "../../common/ikv-common.h"

namespace flyingkv {
namespace acc {
class RwtConfig;
class RWT {
PUBLIC
    void Init(const RwtConfig *conf);
    uint32_t GetWeight(common::ReqRespType rt);

PRIVATE
    std::unordered_map<common::ReqRespType, uint32_t>   m_hmWeightTable;
};
} // namespace acc
} // namespace flyingkv


#endif //FLYINGKV_ACC_RWT_H
