/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_MINI_KV_H
#define MINIKV_MINI_KV_H

#include <map>

#include "../../codegen/meta.pb.h"
#include "../../common/iservice.h"

#include "../ikv-handler.h"

namespace minikv {
namespace sys {
class MemPool;
}
namespace wal {
class IWal;
}
namespace kv {
class EntryComparer
{
    bool operator()(const protocol::Entry *x, const protocol::Entry *y) const {
        return x->key() < y->key();
    }
};

class MiniKV : public common::IService, public IKVHandler {
public:
    MiniKV(std::string &walType, std::string &checkpointType);
    ~MiniKV() override;

    bool Start() override;
    bool Stop() override;

    rpc::SP_PB_MSG OnPut(rpc::SP_PB_MSG sspMsg)    override;
    rpc::SP_PB_MSG OnGet(rpc::SP_PB_MSG sspMsg)    override;
    rpc::SP_PB_MSG OnDelete(rpc::SP_PB_MSG sspMsg) override;
    rpc::SP_PB_MSG OnScan(rpc::SP_PB_MSG sspMsg)   override;

private:
    std::map<protocol::Entry*, protocol::Entry*, EntryComparer> m_kvs;
    sys::MemPool       *m_pMp = nullptr;
    wal::IWal          *m_pWal = nullptr;
};
}
}

#endif //MINIKV_MINI_KV_H
