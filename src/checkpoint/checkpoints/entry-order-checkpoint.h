/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ENTRY_ORDER_CHECKPOINT_H
#define FLYINGKV_ENTRY_ORDER_CHECKPOINT_H

#include <string>

#include "../icheckpoint.h"
#include "../../common/ientry.h"
#include "../config.h"

#define    EOCP_NAME            "entry-order checkpoint"
#define    EOCPLOGEFUN           LOGEFUN << EOCP_NAME
//         EOCP_MAGIC_NO          s m c p
#define    EOCP_MAGIC_NO        0x736d6370
#define    EOCP_MAGIC_NO_LEN    4
#define    EOCP_PREFIX_NAME     "smcp"
#define    EOCP_SAVE_START_FLAG "smcp.start"
#define    EOCP_COMPLETE_FLAG   "smcp.ok"
#define    EOCP_META_SUFFIX     ".meta"
#define    EOCP_NEW_FILE_SUFFIX ".new"
#define    EOCP_LOCK_NAME       ".lock"

#define    EOCP_START_POS_LEN            4
#define    EOCP_SIZE_LEN                 4
#define    EOCP_VERSION_LEN              1
#define    EOCP_ENTRY_HEADER_LEN         5 // EOCP_SIZE_LEN + EOCP_VERSION_LEN
#define    EOCP_CONTENT_OFFSET           5 // EOCP_SIZE_LEN + EOCP_VERSION_LEN
#define    EOCP_ENTRY_EXTRA_FIELDS_SIZE  9 // contentLen(4) + version(1) + startPos(4)
#define    EOCP_INVALID_ENTRY_ID         0

namespace flyingkv {
namespace checkpoint {
/**
 * save cp:
 * - current process:
 *  1. check lock
 *  2. fork child
 *  3. wait child
 *
 * - child process:
 *  1.  create lock file and lock it
 *  2.  check and recover
 *  3.  create start flag file
 *  4.  save checkpoint meta to yyy.new
 *  5.  save checkpoint to xxx.new
 *  6.  create ok flag file
 *  7.  rm start flag file
 *  8.  rm current checkpoint meta file
 *  9.  rm current checkpoint file
 *  10. mv new checkpoint meta yyy.new to yyy
 *  11. mv new checkpoint xxx.new to xxx
 *  12. rm ok flag file
 *  13. unlock and rm lock file
 *  14. exit
 */

class EntryOrderCheckpoint : public ICheckpoint {
PUBLIC
    explicit EntryOrderCheckpoint(const EntryOrderCheckpointConfig *pc);
    ~EntryOrderCheckpoint() override = default;

    CheckpointResult Init() override;
    LoadCheckpointResult Load(EntryLoadedCallback callback) override;
    SaveCheckpointResult Save(IEntriesTraveller *traveller) override;

PRIVATE
    struct LoadMetaResult : public CheckpointResult {
        uint64_t   EntryId;

        explicit LoadMetaResult(uint64_t entryId) : CheckpointResult(Code::OK, ""), EntryId(entryId) {}
        LoadMetaResult(Code code, const std::string &errmsg) : CheckpointResult(code, errmsg) {}
    };

PRIVATE
    SaveCheckpointResult do_save_in_parent(int childPid);
    void do_save_in_child(IEntriesTraveller *traveller);
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
    std::string                 m_sLockPath;
    common::EntryCreateHandler  m_entryCreator;
    uint8_t                     m_writeEntryVersion;
    uint32_t                    m_batchReadSize;
};
}
}

#endif //FLYINGKV_ENTRY_ORDER_CHECKPOINT_H
