/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_WAL_CONFIG_H
#define FLYINGKV_WAL_CONFIG_H

#include <string>
#include <cstdint>

#include "../common/ientry.h"

namespace flyingkv {
namespace wal {
struct WalConfig {
    std::string                Type;
    std::string                RootDirPath;
    common::EntryCreateHandler ECH;

    WalConfig(const std::string &type, const std::string &rootDirPath, const common::EntryCreateHandler &ech) :
        Type(type), RootDirPath(rootDirPath), ECH(ech) {}
};

struct LogCleanWalConfig : public WalConfig {
    uint8_t       WriteEntryVersion;
    uint32_t      MaxSegmentSize;
    uint32_t      ReadBatchSize;

    LogCleanWalConfig(const std::string &type, const std::string &rootDirPath, const common::EntryCreateHandler &ech,
                        uint8_t writeEntryVersion, uint32_t maxSegmentSize, uint32_t readBatchSize) :
            WalConfig(type, rootDirPath, ech), WriteEntryVersion(writeEntryVersion), MaxSegmentSize(maxSegmentSize),
            ReadBatchSize(readBatchSize) {}
};
}
}

#endif //FLYINGKV_WAL_CONFIG_H
