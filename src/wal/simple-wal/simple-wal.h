/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_SIMPLE_WAL_H
#define MINIKV_SIMPLE_WAL_H

#include <unordered_map>

#include "../iwal.h"

//         SMLOG_MAGIC_NO        s m l g
#define    SMLOG_MAGIC_NO      0x736d6c67
#define    SMLOG_MAGIC_NO_LEN  4
#define    SMLOG_PREFIX_NAME   "smlog"

namespace minikv {
namespace wal {
// TODO(sunchao): 1. add batch sync feature with autoSync conf? 2. with multi files?
/**
 * Not thread-safe!
 * log format on disk file(file header + log segments)：
 *  |<-log file header(magic no)->|
 *  |<-log content len(u64)->|<-log content->|<-log segment start pos(u64)->|
 *   ...(log segments)...
 *  |<-log content len(u64)->|<-log content->|<-log segment start pos(u64)->|
 */
class SimpleWal : public IWal {
public:
    SimpleWal(std::string &rootDir);
    ~SimpleWal() override;

    void AppendEntry(IEntry *entry) override;
    std::vector<WalEntry> Load() override;
    void TruncateAhead() override;
    void Clean() override;

private:
    std::string                           m_sRootDir;
    std::unordered_map<uint64_t, int64_t> m_mpEntriesIdOffset;
    uint64_t                              m_iCurrentLogIdx;
    std::string                           m_sLogFilePath;
    int                                   m_iFd           = -1;
    uint32_t                              m_iCurIdxToSync = 0;
    uint32_t                              m_iFileSize     = 0;
};
}
}

#endif //MINIKV_SIMPLE_WAL_H
