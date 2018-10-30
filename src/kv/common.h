/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_COMMON_H
#define FLYINGKV_KV_COMMON_H

#include <unordered_map>
#include <string>

#include "../common/ikv-common.h"

namespace flyingkv {
namespace kv {
struct EngineConstructorParams {
    std::string Type;
    std::string WalType;
    std::string WalDir;
    std::string CheckpointType;
    std::string CheckpointDir;
    std::string AccConfPath;
    std::unordered_map<common::ReqRespType, int64_t> ReqTimeoutMs;
};
}
}

#endif //FLYINGKV_KV_COMMON_H
