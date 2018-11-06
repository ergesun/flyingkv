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
#include "../../wal/iwal.h"
#include "../../checkpoint/ientries-traveller.h"

namespace flyingkv {
namespace acc {
class IGranter;
}
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
class KVConfig;
class EntryComparer
{
    bool operator()(const protocol::Entry *x, const protocol::Entry *y) const {
        return x->key() < y->key();
    }
};

class MiniKV : public common::IService, public common::IKVOperator, public checkpoint::IEntriesTraveller {
PUBLIC
    MiniKV(const KVConfig *pc);
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

    uint64_t MaxId() override;

PRIVATE
    common::IEntry* create_new_entry();

    void on_checkpoint_load_entry(std::vector<common::IEntry*> entries);
    void on_wal_load_entries(std::vector<wal::WalEntry>);

PRIVATE
    std::map<protocol::Entry*, protocol::Entry*, EntryComparer> m_kvs;
    sys::MemPool                 *m_pMp = nullptr;
    wal::IWal                    *m_pWal = nullptr;
    checkpoint::ICheckpoint      *m_pCheckpoint = nullptr;
    acc::IGranter                *m_pGranter = nullptr;
};
}
}

#endif //FLYINGKV_MINI_KV_H
