/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../3rd/cjson/cJSON.h"

#include "../common/common-def.h"

#include "rwt.h"

namespace flyingkv {
namespace acc {
bool RWT::Parse(cJSON *blockRoot) {
    auto putItem = cJSON_GetObjectItem(blockRoot, "put");
    m_hmWeightTable[common::ReqRespType::Put] = (uint32_t)putItem->valueint;
    auto getItem = cJSON_GetObjectItem(blockRoot, "get");
    m_hmWeightTable[common::ReqRespType::Get] = (uint32_t)getItem->valueint;
    auto listItem = cJSON_GetObjectItem(blockRoot, "delete");
    m_hmWeightTable[common::ReqRespType::Delete] = (uint32_t)listItem->valueint;
    auto deleteItem = cJSON_GetObjectItem(blockRoot, "scan");
    m_hmWeightTable[common::ReqRespType::Scan] = (uint32_t)deleteItem->valueint;

    return true;
}

uint32_t RWT::GetWeight(common::ReqRespType rt) {
    if (m_hmWeightTable.end() == m_hmWeightTable.find(rt)) {
        // For all request types that we don not match, default 1 weight
        return 1;
    }

    return m_hmWeightTable[rt];
}
} // namespace acc
} // namespace flyingkv
