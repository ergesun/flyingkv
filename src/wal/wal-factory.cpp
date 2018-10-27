/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "wal-factory.h"
#include "simple-wal/simple-wal.h"

namespace minikv {
namespace wal {
std::unordered_map<std::string, int> g_typeMapper = std::unordered_map<std::string, int> {
        {"simple", 0}
};

IWal* WALFactory::CreateWalInstance(const std::string &type, std::string &rootDir, EntryCreateHandler &&handler) {
    auto rs = g_typeMapper.find(type);
    if (rs == g_typeMapper.end()) {
        LOGFFUN << "cannot find wal class type " << type;
    }
    switch (rs->second) {
    case 0:
        return new SimpleWal(rootDir, std::move(handler));
    default:
        return nullptr;
    }
}
}
}
