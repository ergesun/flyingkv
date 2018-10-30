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
#include "../../acc/igranter.h"

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
namespace kv {
class EntryComparer
{
    bool operator()(const protocol::Entry *x, const protocol::Entry *y) const {
        return x->key() < y->key();
    }
};

class MiniKV : public common::IService, public common::IKVOperator, public checkpoint::IEntriesTraveller {
PUBLIC
    MiniKV(std::string &walType, std::string &walDir, std::string &checkpointType,
           std::string &checkpointDir, std::string &accConfPath, std::unordered_map<common::ReqRespType, int64_t> &reqTimeout);
    ~MiniKV() override;

    bool Start() override;
    bool Stop() override;

    // kv operator
    common::SP_PB_MSG Put(common::KVPutRequest)    override;
    common::SP_PB_MSG Get(common::KVGetRequest)    override;
    common::SP_PB_MSG Delete(common::KVDeleteRequest) override;
    common::SP_PB_MSG Scan(common::KVScanRequest)   override;

    // entries traveller
    common::IEntry *GetNextEntry() override;
    bool Empty() override;

PRIVATE
    common::IEntry* create_new_entry();

    void on_checkpoint_load_entry(common::IEntry*);
    void on_wal_load_entries(std::vector<wal::WalEntry>&);

PRIVATE
    std::map<protocol::Entry*, protocol::Entry*, EntryComparer> m_kvs;
    sys::MemPool                 *m_pMp = nullptr;
    wal::IWal                    *m_pWal = nullptr;
    checkpoint::ICheckpoint      *m_pCheckpoint = nullptr;
    acc::IGranter                *m_pGranter = nullptr;
    std::unordered_map<common::ReqRespType, int64_t> m_hmReqTimeoutMs;
};
}
}

#endif //FLYINGKV_MINI_KV_H
