/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_SIMPLE_WAL_H
#define FLYINGKV_SIMPLE_WAL_H

#include <unordered_map>

#include "../iwal.h"
#include "../../sys/gcc-buildin.h"

//         SMLOG_MAGIC_NO                   s m l g
#define    SMLOG_MAGIC_NO                 0x736d6c67
#define    SMLOG_MAGIC_NO_LEN             4
#define    SMLOG_PREFIX_NAME              "smlog"

#define    SMLOG_START_POS_LEN            4
#define    SMLOG_SIZE_LEN                 4
#define    SMLOG_ENTRY_ID_LEN             8
#define    SMLOG_ENTRY_EXTRA_FIELDS_SIZE  16 // contentLen(4) + logId(8) + startPos(4)

namespace flyingkv {
namespace common {
class IEntry;
}
namespace wal {
// TODO(sunchao): 1. add batch sync feature with autoSync conf? 2. with multi files?
/**
 * Not thread-safe!
 * CASE 'LOG format version == 1':
 *   log id = [log version(u32) + entry id(u32)]
 *   log format on disk file(file header + log segments)：
 *    |<-log file header(magic no)->|
 *    |<-log content len(u32)->|<-log format verion(u8)->|<-log entry id(u64)->|<-log content->|<-log segment start pos(u32)->|
 *     ...(log segments)...
 *    |<-log content len(u32)->|<-log format verion(u8)->|<-log entry id(u64)->|<-log content->|<-log segment start pos(u32)->|
 */
class SimpleWal : public IWal {
PUBLIC
    SimpleWal(std::string &rootDir, common::EntryCreateHandler &&handler);
    ~SimpleWal() override;

    uint64_t AppendEntry(common::IEntry *entry) override;
    std::vector<WalEntry> Load() override;
    bool TruncateAhead(uint64_t id) override;
    void Reset() override;

PRIVATE
    void load_log_version();

PRIVATE
    std::string                           m_sRootDir;
    std::unordered_map<uint64_t, int64_t> m_mpEntriesIdEndOffset;
    // 增长时无需考虑溢出，因为没等溢出version就会增长
    uint64_t                              m_currentLogIdx;
    uint64_t                              m_lastTruncLogIdx;
    std::string                           m_sLogFilePath;
    int                                   m_fd             = -1;
    uint32_t                              m_fileSize       = 0;
    common::EntryCreateHandler            m_entryCreator;
    bool                                  m_bLoaded        = false;
    // 增长时无需考虑溢出，能活到那么久么。。。如果真活到那么久，由于日志有version，可以到时候扩展为64位的，甚至扩展为时间戳
    uint32_t                              m_logFileVersion = 0;
};
}
}

#endif //FLYINGKV_SIMPLE_WAL_H
