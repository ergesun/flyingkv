/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_RWT_H
#define FLYINGKV_ACC_RWT_H

#include <unordered_map>
#include <string>

#include "../common/ikv-common.h"
#include "conf-parser.h"

namespace std {
// 不可以去掉，std::unordered_map会使用它来hash RequestType
template<>
struct hash<flyingkv::common::ReqRespType> {
    uint32_t operator()(const flyingkv::common::ReqRespType &rt) const {
        return (uint32_t)rt;
    }
};
}

namespace flyingkv {
namespace acc {
class RWT : public IConfParser {
public:
    bool Parse(cJSON *blockRoot) override;
    uint32_t GetWeight(common::ReqRespType rt);

private:
    std::unordered_map<common::ReqRespType, uint32_t>   m_hmWeightTable;
};
} // namespace acc
} // namespace flyingkv


#endif //FLYINGKV_ACC_RWT_H
