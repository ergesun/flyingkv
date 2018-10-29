/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <unordered_map>

#include "../common/common-def.h"

#include "checkpoint-factory.h"
#include "checkpoints/simple-checkpoint.h"

static std::unordered_map<std::string, int> g_typeMapper = std::unordered_map<std::string, int> {
        {"simple", 0}
};

namespace flyingkv {
namespace checkpoint {
ICheckpoint* CheckpointFactory::CreateInstance(const std::string &type, std::string &rootDir, common::EntryCreateHandler &&handler) {
    auto rs = g_typeMapper.find(type);
    if (rs == g_typeMapper.end()) {
        LOGFFUN << "cannot find checkpoint class type " << type;
    }
    switch (rs->second) {
        case 0:
            return new SimpleCheckpoint(rootDir, std::move(handler));
        default:
            return nullptr;
    }
}
}
}
