/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../3rd/cjson/cJSON.h"

#include "../../common/common-def.h"
#include "../config.h"

#include "rwt.h"

namespace flyingkv {
namespace acc {
void RWT::Init(const RwtConfig *conf) {
    m_hmWeightTable[common::ReqRespType::Put] = conf->Put;
    m_hmWeightTable[common::ReqRespType::Get] = conf->Get;
    m_hmWeightTable[common::ReqRespType::Delete] = conf->Delete;
    m_hmWeightTable[common::ReqRespType::Scan] = conf->Scan;
}

uint32_t RWT::GetWeight(common::ReqRespType rt) {
    auto rs = m_hmWeightTable.find(rt);
    if (m_hmWeightTable.end() == rs) {
        // For all request types that we don not match, default 1 weight
        return 1;
    }

    return rs->second;
}
} // namespace acc
} // namespace flyingkv
