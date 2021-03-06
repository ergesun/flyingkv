/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <unordered_map>

#include "../common/common-def.h"

#include "wal-factory.h"
#include "wals/log-clean-wal.h"

namespace flyingkv {
namespace wal {
static std::unordered_map<std::string, int> g_typeMapper = std::unordered_map<std::string, int> {
        {"log-clean", 0}
};

IWal* WALFactory::CreateInstance(const WalConfig *pc) {
    auto rs = g_typeMapper.find(pc->Type);
    if (rs == g_typeMapper.end()) {
        LOGEFUN << "cannot find wal class type " << pc->Type;
        return nullptr;
    }
    switch (rs->second) {
    case 0:
        return new LogCleanWal(static_cast<const LogCleanWalConfig*>(pc));
    default:
        return nullptr;
    }
}
}
}
