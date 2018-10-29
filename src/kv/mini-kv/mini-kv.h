/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_MINI_KV_H
#define FLYINGKV_MINI_KV_H

#include <map>

#include "../../codegen/meta.pb.h"
#include "../../common/iservice.h"

#include "../../common/ikv-common.h"
#include "../../common/blocking-queue.h"

namespace flyingkv {
namespace common {
class IEntry;
}
namespace sys {
class MemPool;
}
namespace wal {
class IWal;
}
namespace checkpoint {
class ICheckpoint;
}
namespace minikv {
class EntryComparer
{
    bool operator()(const protocol::Entry *x, const protocol::Entry *y) const {
        return x->key() < y->key();
    }
};

class MiniKV : public common::IService, public common::IKVHandler {
PUBLIC
    MiniKV(std::string &walType, std::string &checkpointType,
           std::string &walDir, std::string &checkpointDir, uint32_t maxPendingCnt);
    ~MiniKV() override;

    bool Start() override;
    bool Stop() override;

    common::SP_PB_MSG OnPut(common::KVPutRequest)    override;
    common::SP_PB_MSG OnGet(common::KVGetRequest)    override;
    common::SP_PB_MSG OnDelete(common::KVDeleteRequest) override;
    common::SP_PB_MSG OnScan(common::KVScanRequest)   override;

PRIVATE
    common::IEntry* create_new_entry();

PRIVATE
    common::BlockingQueue<common::SP_PB_MSG>  *m_pbqPendingTasks;
    std::map<protocol::Entry*, protocol::Entry*, EntryComparer> m_kvs;
    sys::MemPool                 *m_pMp = nullptr;
    wal::IWal                    *m_pWal = nullptr;
    checkpoint::ICheckpoint      *m_pCheckpoint = nullptr;
};
}
}

#endif //FLYINGKV_MINI_KV_H
