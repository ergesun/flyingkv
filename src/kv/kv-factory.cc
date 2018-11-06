/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "mini-kv/mini-kv.h"
#include "config.h"

#include "kv-factory.h"

namespace flyingkv {
namespace kv {
static std::unordered_map<std::string, int> g_typeMapper = std::unordered_map<std::string, int> {
    {"mini", 0}
};

common::IKVOperator* KVFactory::CreateInstance(const KVConfig *pc) {
    auto rs = g_typeMapper.find(pc->Type);
    if (rs == g_typeMapper.end()) {
        LOGEFUN << "cannot find kv class type " << pc->Type;
        return nullptr;
    }
    switch (rs->second) {
        case 0:
            return new MiniKV(pc);
        default:
            return nullptr;
    }
}
}
}
