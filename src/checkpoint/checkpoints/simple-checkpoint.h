/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_SIMPLE_CHECKPOINT_H
#define MINIKV_SIMPLE_CHECKPOINT_H

#include <string>

#include "../icheckpoint.h"
#include "../../common/ientry.h"

//         SMCP_MAGIC_NO          s m c p
#define    SMCP_MAGIC_NO        0x736d6370
#define    SMCP_MAGIC_NO_LEN    4
#define    SMCP_PREFIX_NAME     "smcp"
#define    SMCP_COMPLETE_FLAG   "smcp.ok"
#define    SMCP_NEW_FILE_SUFFIX ".new"

namespace minikv {
namespace checkpoint {
class SimpleCheckpoint : public ICheckpoint {
public:
    explicit SimpleCheckpoint(const std::string &rootDir, common::EntryCreateHandler &&ech);
    ~SimpleCheckpoint() override;

    bool Load(EntryLoadedCallback callback) override;
    bool Save(IEntriesTraveller *traveller) override;

private:
    int create_new_checkpoint();
    inline bool is_completed();
    inline bool rm_completed_status();
    inline bool create_completed_status();

private:
    std::string                           m_sRootDir;
    std::string                           m_sCpFilePath;
    std::string                           m_sNewLogFilePath;
    std::string                           m_sStatusFilePath;
    common::EntryCreateHandler            m_entryCreator;
};
}
}

#endif //MINIKV_SIMPLE_CHECKPOINT_H