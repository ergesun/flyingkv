/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_SIMPLE_CHECKPOINT_H
#define FLYINGKV_SIMPLE_CHECKPOINT_H

#include <string>

#include "../icheckpoint.h"
#include "../../common/ientry.h"

#define    SMCP_NAME            "simple checkpoint"
#define    SMCPLOGEFUN           LOGEFUN << SMCP_NAME
//         SMCP_MAGIC_NO          s m c p
#define    SMCP_MAGIC_NO        0x736d6370
#define    SMCP_MAGIC_NO_LEN    4
#define    SMCP_PREFIX_NAME     "smcp"
#define    SMCP_SAVE_START_FLAG "smcp.start"
#define    SMCP_COMPLETE_FLAG   "smcp.ok"
#define    SMCP_META_SUFFIX     ".meta"
#define    SMCP_NEW_FILE_SUFFIX ".new"

#define    SMCP_VERSION                  1 // TODO(sunchao):可配置？
#define    SMCP_START_POS_LEN            4
#define    SMCP_SIZE_LEN                 4
#define    SMCP_VERSION_LEN              1
#define    SMCP_ENTRY_HEADER_LEN         13 // SMLOG_SIZE_LEN + SMLOG_VERSION_LEN
#define    SMCP_CONTENT_OFFSET           13 // SMLOG_SIZE_LEN + SMLOG_VERSION_LEN
#define    SMCP_ENTRY_EXTRA_FIELDS_SIZE  17 // contentLen(4) + version(1) + startPos(4)
#define    SMCP_INVALID_ENTRY_ID         0

const uint32_t SMCP_READ_BATCH_SIZE  = 1024 * 1024 * 10;  // 10MiB

namespace flyingkv {
namespace checkpoint {
/**
 * save cp:
 *   1.  create start flag file
 *   2.  save checkpoint meta to yyy.new
 *   3.  save checkpoint to xxx.new
 *   4.  create ok flag file
 *   5.  rm start flag file
 *   6.  rm current checkpoint meta file
 *   7.  rm current checkpoint file
 *   8.  mv new checkpoint meta yyy.new to yyy
 *   9.  mv new checkpoint xxx.new to xxx
 *   10. rm ok flag file
 */

class SimpleCheckpoint : public ICheckpoint {
PUBLIC
    explicit SimpleCheckpoint(const std::string &rootDir, common::EntryCreateHandler &&ech);
    ~SimpleCheckpoint() override = default;

    CheckpointResult Init() override;
    LoadCheckpointResult Load(EntryLoadedCallback callback) override;
    CheckpointResult Save(IEntriesTraveller *traveller) override;

PRIVATE
    struct LoadMetaResult : public CheckpointResult {
        uint64_t   EntryId;

        LoadMetaResult(Code code, const std::string &errmsg) : CheckpointResult(code, errmsg) {}
        LoadMetaResult(uint64_t entryId) : CheckpointResult(Code::OK, ""), EntryId(entryId) {}
    };

PRIVATE
    CheckpointResult init_new_checkpoint(uint64_t id);
    inline bool is_completed();
    CheckpointResult create_ok_flag();
    CheckpointResult check_and_recover();
    CheckpointResult clean_new_cp_tmp();
    CheckpointResult replace_old_checkpoint();
    LoadMetaResult   load_meta();
    CheckpointResult create_new_meta_file(uint64_t id);
    CheckpointResult create_new_checkpoint_file();
    CheckpointResult save_new_checkpoint(IEntriesTraveller *traveller);

PRIVATE
    std::string                 m_sRootDir;
    bool                        m_bInited          = false;
    std::string                 m_sCpFilePath;
    std::string                 m_sNewCpFilePath;
    std::string                 m_sCpMetaFilePath;
    std::string                 m_sNewCpMetaFilePath;
    std::string                 m_sNewStartFlagFilePath;
    std::string                 m_sNewCpSaveOkFilePath;
    common::EntryCreateHandler  m_entryCreator;
};
}
}

#endif //FLYINGKV_SIMPLE_CHECKPOINT_H
