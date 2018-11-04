/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_SIMPLE_WAL_H
#define FLYINGKV_SIMPLE_WAL_H

#include <unordered_map>

#include "../iwal.h"
#include "../../sys/gcc-buildin.h"

#define    LOGCLEAN_WAL_NAME                    "log-clean"
#define    LCLOGEFUN                            LOGEFUN << LOGCLEAN_WAL_NAME

//         LOGCLEAN_WAL_MAGIC_NO                   s m l g
#define    LOGCLEAN_WAL_MAGIC_NO                 0x736d6c67
#define    LOGCLEAN_WAL_MAGIC_NO_LEN             4
#define    LOGCLEAN_WAL_SEGMENT_PREFIX_NAME      "smlog.segment"
#define    LOGCLEAN_WAL_TRUNC_OK_NAME            "smlog.trunc.ok"
#define    LOGCLEAN_WAL_TRUNC_META_NAME          "smlog.truncmeta"

#define    LOGCLEAN_WAL_VERSION                  1 // TODO(sunchao):可配置？
#define    LOGCLEAN_WAL_START_POS_LEN            4
#define    LOGCLEAN_WAL_SIZE_LEN                 4
#define    LOGCLEAN_WAL_VERSION_LEN              1
#define    LOGCLEAN_WAL_ENTRY_ID_LEN             8
#define    LOGCLEAN_WAL_ENTRY_ID_OFFSET          5  // SMLOG_SIZE_LEN + SMLOG_VERSION_LEN
#define    LOGCLEAN_WAL_ENTRY_HEADER_LEN         13 // SMLOG_SIZE_LEN + SMLOG_VERSION_LEN + SMLOG_ENTRY_ID_LEN
#define    LOGCLEAN_WAL_CONTENT_OFFSET           13 // SMLOG_SIZE_LEN + SMLOG_VERSION_LEN + SMLOG_ENTRY_ID_LEN
#define    LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE  17 // contentLen(4) + version(1) + logId(8) + startPos(4)
#define    LOGCLEAN_WAL_INVALID_SEGMENT_ID       0
#define    LOGCLEAN_WAL_INVALID_ENTRY_ID         0
#define    LOGCLEAN_WAL_MIN_ENTRY_ID             1

// TODO(sunchao): 可配置？
const uint32_t  LOGCLEAN_WAL_SEGMENT_MAX_SIZE = 1024 * 1024 * 100; // 100MiB
const uint32_t  LOGCLEAN_WAL_READ_BATCH_SIZE  = 1024 * 1024 * 10;  // 10MiB
const uint32_t  LOGCLEAN_WAL_ENTRY_MAX_SIZE   =  LOGCLEAN_WAL_SEGMENT_MAX_SIZE - LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;

namespace flyingkv {
namespace common {
class IEntry;
}
namespace wal {
// TODO(sunchao): 1. add batch sync feature with autoSync conf?
/**
 * Not thread-safe!
 *
 * - CASE 'LOG format version == 1':
 *   log id = [log version(u32) + entry id(u32)]
 *   log format on disk file(file header + log segments)：
 *    |<-log file header(magic no)->|
 *    |<-log content len(u32)->|<-log format version(u8)->|<-log entry id(u64)->|<-log content->|<-log segment start pos(u32)->|
 *     ...(log segments)...
 *    |<-log content len(u32)->|<-log format version(u8)->|<-log entry id(u64)->|<-log content->|<-log segment start pos(u32)->|
 *
 *
 * - CASE 'LOG format version == 2': TODO(sunchao):
 *   log id = [log version(u32) + entry id(u32)]
 *   log format on disk file(file header + log segments)：
 *    |<-log file header(magic no)->|
 *    |<-log content len(u32)->|<-log format version(u8)->|<-log entry id(u64)->|<-compressed log content->|<-log segment start pos(u32)->|
 *     ...(log segments)...
 *    |<-log content len(u32)->|<-log format version(u8)->|<-log entry id(u64)->|<-compressed log content->|<-log segment start pos(u32)->|
 *
 */

struct LogCleanWalSegmentFileInfo {
    uint64_t    Id;
    std::string Path;
};

class LogCleanWal : public IWal {
PUBLIC
    LogCleanWal(const std::string &rootDir, common::EntryCreateHandler &&handler);
    ~LogCleanWal() override;

    WalResult Init() override;
    /**
     * 向version最大的日志文件追加
     * @param entry
     * @return
     */
    AppendEntryResult AppendEntry(common::IEntry *entry) override;
    LoadResult Load(const WalEntryLoadedCallback&) override;
    TruncateResult Truncate(uint64_t id) override;

PRIVATE
    struct LoadSegmentResult : public WalResult {
        uint32_t FileSize;

        explicit LoadSegmentResult(uint32_t fs) : WalResult(Code::OK), FileSize(fs) {}
        LoadSegmentResult(Code rc, const std::string &errmsg) : WalResult(rc, errmsg), FileSize(0) {}
    };

    struct ListSegmentsResult : public WalResult {
        std::vector<LogCleanWalSegmentFileInfo> Segments;

        explicit ListSegmentsResult(std::vector<LogCleanWalSegmentFileInfo> &&segments) : WalResult(Code::OK), Segments(std::move(segments)) {}
        ListSegmentsResult(Code rc, const std::string &errmsg) : WalResult(rc, errmsg) {}
    };

    typedef AppendEntryResult LoadSegmentMaxEntryIdResult;

PRIVATE
    /**
     *
     * @param filePath
     * @param segId
     * @param callback
     * @return file size
     */
    LoadSegmentResult load_segment(const std::string &filePath, uint64_t segId, const WalEntryLoadedCallback &callback);
    ListSegmentsResult list_segments_asc();
    uint64_t get_uncompleted_trunc_segments_max_id();
    std::string generate_segment_file_path(uint64_t id);
    WalResult clean_trunc_status();
    WalResult create_new_segment_file();
    WalResult write_trunc_info(uint64_t segId);
    WalResult write_trunc_ok_flag();
    LoadSegmentMaxEntryIdResult load_segment_max_entry_id(const std::string &fp);

PRIVATE
    std::string                               m_sRootDir;
    std::unordered_map<uint64_t, uint64_t>    m_mpEntriesIdSegId;
    uint64_t                                  m_lastTruncMaxIdx = LOGCLEAN_WAL_MIN_ENTRY_ID;
    // 增长时无需考虑溢出，因为没等溢出version就会增长
    uint64_t                                  m_currentEntryIdx = LOGCLEAN_WAL_MIN_ENTRY_ID;
    int                                       m_currentSegFd         = -1;
    std::string                               m_currentSegFilePath;
    uint32_t                                  m_curSegFileSize       = 0;
    common::EntryCreateHandler                m_entryCreator;
    bool                                      m_bLoaded          = false;
    // 增长时无需考虑溢出，能活到那么久么。。。如果真活到那么久，由于日志有version，可以到时候扩展为64位的，甚至扩展为时间戳
    uint64_t                                  m_currentSegmentId = 0;
    uint64_t                                  m_minSegmentId     = 0;
    std::string                               m_sTruncOKFlagFilePath;
    std::string                               m_sTruncInfoFilePath;
    std::vector<LogCleanWalSegmentFileInfo>   m_vInitSegments;
};
}
}

#endif //FLYINGKV_SIMPLE_WAL_H
