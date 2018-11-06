/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_CONFIG_H
#define FLYINGKV_KV_CONFIG_H

#include <string>

namespace flyingkv {
namespace kv {
struct KVConfig {
    std::string Type;
    std::string AccConfPath;
    std::string WalType;
    std::string WalRootDirPath;
    uint8_t     WalWriteEntryVersion;
    uint32_t    WalMaxSegmentSize;
    uint32_t    WalReadBatchSize;
    std::string CheckpointType;
    std::string CheckpointRootDirPath;
    uint8_t     CheckpointWriteEntryVersion;
    uint32_t    CheckpointReadBatchSize;
};

//typedef KVConfig MiniKVConfig;
}
}

#endif //FLYINGKV_KV_CONFIG_H
