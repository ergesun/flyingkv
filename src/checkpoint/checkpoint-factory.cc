/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <unordered_map>

#include "../common/common-def.h"

#include "checkpoint-factory.h"
#include "checkpoints/entry-order-checkpoint.h"

static std::unordered_map<std::string, int> g_typeMapper = std::unordered_map<std::string, int> {
        {"entry-order", 0}
};

namespace flyingkv {
namespace checkpoint {
ICheckpoint* CheckpointFactory::CreateInstance(const CheckpointConfig *pc) {
    auto rs = g_typeMapper.find(pc->Type);
    if (rs == g_typeMapper.end()) {
        LOGEFUN << "cannot find checkpoint class type " << pc->Type;
        return nullptr;
    }
    switch (rs->second) {
        case 0:
            return new EntryOrderCheckpoint(static_cast<const EntryOrderCheckpointConfig*>(pc));
        default:
            return nullptr;
    }
}
}
}
