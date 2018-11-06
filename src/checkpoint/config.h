/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_CHECKPOINT_CONFIG_H
#define FLYINGKV_CHECKPOINT_CONFIG_H

#include <string>

#include "../common/ientry.h"

namespace flyingkv {
namespace checkpoint {
struct CheckpointConfig {
    std::string                Type;
    std::string                RootDirPath;
    common::EntryCreateHandler ECH;

    CheckpointConfig(const std::string &type, const std::string &rootDirPath, const common::EntryCreateHandler &ech) :
    Type(type), RootDirPath(rootDirPath), ECH(ech) {}
};

struct EntryOrderCheckpointConfig : public CheckpointConfig {
    uint8_t       WriteEntryVersion;
    uint32_t      BatchReadSize;

    EntryOrderCheckpointConfig(const std::string &type, const std::string &rootDirPath,
                                const common::EntryCreateHandler &ech,
                                uint8_t writeEntryVersion, uint32_t readBatchSize) :
                                CheckpointConfig(type, rootDirPath, ech), WriteEntryVersion(writeEntryVersion),
                                BatchReadSize(readBatchSize) {}
};
}
}

#endif //FLYINGKV_CHECKPOINT_CONFIG_H
