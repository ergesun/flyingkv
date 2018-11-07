/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_MINI_KV_H
#define FLYINGKV_MINI_KV_H

#include <map>
#include <mutex>

#include "../../codegen/meta.pb.h"
#include "../../common/iservice.h"
#include "../../common/ikv-common.h"
#include "../../wal/iwal.h"
#include "../../checkpoint/ientries-traveller.h"
#include "../../sys/rw-mutex.h"

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
class RawPbEntryEntry;
class MiniKV : public common::IService, public common::IKVOperator, public checkpoint::IEntriesTraveller {
PUBLIC
    explicit MiniKV(const KVConfig *pc);
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

    struct MiniKVKey {
        const uchar *Data;
        uint32_t     Len;

        // 这里不考虑空值，minikv会有空key检查
        bool operator<(const MiniKVKey &b) const {
            auto minLen = this->Len < b.Len ? this->Len : b.Len;
            auto lessThanB = this->Len < b.Len;
            auto rs = memcmp(this->Data, b.Data, minLen);
            if (rs < 0) {
                return true;
            } else if (rs == 0) {
                return lessThanB;
            } else {
                return false;
            }
        }
    };

PRIVATE
    common::IEntry* create_wal_new_entry(const common::Buffer &b);
    common::IEntry* create_checkpoint_new_entry(const common::Buffer &b);

    void on_checkpoint_load_entry(std::vector<common::IEntry*> entries);
    void on_wal_load_entries(std::vector<wal::WalEntry>);

PRIVATE
    bool                                  m_bStopped = true;
    std::map<MiniKVKey, RawPbEntryEntry*> m_kvs;
    sys::RwMutex                          m_kvLock;
    sys::MemPool                         *m_pMp = nullptr;
    wal::IWal                            *m_pWal = nullptr;
    checkpoint::ICheckpoint              *m_pCheckpoint = nullptr;
    // TODO(sunchao): 在目前的同步rpc调用模型下acc作用有限(线程个数本身就有比较好的限制)，将来使用异步模型加上scheduler之后效果会好起来
    acc::IGranter                        *m_pGranter = nullptr;
    std::mutex                            m_walLock;
    uint32_t                              m_triggerCheckWalSizeTickMs;
    uint32_t                              m_triggerCheckpointWalSizeMB;
};
}
}

#endif //FLYINGKV_MINI_KV_H
