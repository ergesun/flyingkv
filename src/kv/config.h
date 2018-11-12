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
    std::string   Type;
    std::string   AccConfPath;
    std::string   WalType;
    std::string   WalRootDirPath;
    uint8_t       WalWriteEntryVersion;
    uint32_t      WalMaxSegmentSize;
    uint32_t      WalReadBatchSize;
    std::string   CheckpointType;
    std::string   CheckpointRootDirPath;
    uint8_t       CheckpointWriteEntryVersion;
    uint32_t      CheckpointReadBatchSize;
    uint32_t      CheckWalSizeTickSeconds;
    uint32_t      DoCheckpointWalSizeMB;

    KVConfig(const std::string &type, const std::string &accConfPath, const std::string &walType,
                    const std::string &walRoot, uint8_t walWriteEntryVersion, uint32_t walMaxSegmentSize,
                    uint32_t walReadBatchSize, const std::string &cpType, const std::string &cpRoot,
                    uint8_t cpWriteEntryVersion, uint32_t cpReadBatchSize, uint32_t checkWalSizeTick,
                    uint32_t doCpWalSize) : Type(type), AccConfPath(accConfPath), WalType(walType),
                                            WalRootDirPath(walRoot), WalWriteEntryVersion(walWriteEntryVersion),
                                            WalMaxSegmentSize(walMaxSegmentSize), WalReadBatchSize(walReadBatchSize),
                                            CheckpointType(cpType), CheckpointRootDirPath(cpRoot),
                                            CheckpointWriteEntryVersion(cpWriteEntryVersion), CheckpointReadBatchSize(cpReadBatchSize),
                                            CheckWalSizeTickSeconds(checkWalSizeTick), DoCheckpointWalSizeMB(doCpWalSize) {}
};

//typedef KVConfig MiniKVConfig;
}
}

#endif //FLYINGKV_KV_CONFIG_H
