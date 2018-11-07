/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../sys/gcc-buildin.h"
#include "../../common/ientry.h"
#include "../../wal/wal-factory.h"
#include "../../checkpoint/checkpoint-factory.h"
#include "../../acc/config.h"
#include "../../acc/resource-limit-control/simple-rlc.h"
#include "../../checkpoint/config.h"
#include "../../wal/config.h"
#include "../../common/buffer.h"

#include "../config.h"
#include "../entries/raw-pb-entry-entry.h"
#include "../entries/common.h"
#include "../entries/wal-put-entry.h"
#include "../entries/wal-delete-entry.h"
#include "errors.h"

#include "mini-kv.h"

namespace flyingkv {
namespace kv {
MiniKV::MiniKV(const KVConfig *pc) {
    m_pMp = new sys::MemPool();
    auto pWalConf = new wal::LogCleanWalConfig(pc->WalType, pc->WalRootDirPath, std::bind(&MiniKV::create_wal_new_entry, this, std::placeholders::_1),
                    pc->WalWriteEntryVersion, pc->WalMaxSegmentSize, pc->WalReadBatchSize);
    m_pWal = wal::WALFactory::CreateInstance(pWalConf);
    delete pWalConf;
    if (m_pWal == nullptr) {
        LOGFFUN << " invalid wal configuration";
    }
    auto pCheckpointConf = new checkpoint::EntryOrderCheckpointConfig(pc->CheckpointType, pc->CheckpointRootDirPath,
                                                                      std::bind(&MiniKV::create_checkpoint_new_entry, this, std::placeholders::_1),
                                                pc->CheckpointWriteEntryVersion, pc->CheckpointReadBatchSize);
    m_pCheckpoint = checkpoint::CheckpointFactory::CreateInstance(pCheckpointConf);
    delete pCheckpointConf;
    if (m_pCheckpoint == nullptr) {
        LOGFFUN << " invalid checkpoint configuration";
    }
    auto accConf = acc::ParseAccConfig(pc->AccConfPath);
    auto rlc = new acc::SimpleRlc();
    if (!rlc->Init(accConf)) {
        LOGFFUN << "invalid acc configuration.";
    }
    delete accConf;
    m_pGranter = rlc;
}

MiniKV::~MiniKV() {
    DELETE_PTR(m_pMp);
    DELETE_PTR(m_pWal);
    DELETE_PTR(m_pCheckpoint);
    DELETE_PTR(m_pGranter);
}

bool MiniKV::Start() {
    auto cpInitRs = m_pCheckpoint->Init();
    if (checkpoint::Code::OK != cpInitRs.Rc) {
        return false;
    }

    auto cpLoadRs = m_pCheckpoint->Load(std::bind(&MiniKV::on_checkpoint_load_entry, this, std::placeholders::_1));
    if (checkpoint::Code::OK != cpLoadRs.Rc) {
        return false;
    }

    auto walInitRs = m_pWal->Init();
    if (wal::Code::OK != walInitRs.Rc) {
        return false;
    }

    auto walLoadRs = m_pWal->Load(std::bind(&MiniKV::on_wal_load_entries, this, std::placeholders::_1));

    auto ok = wal::Code::OK == walLoadRs.Rc;
    if (ok) {
        m_bStopped = false;
        hw_sw_memory_barrier();
    }
    return ok;
}

bool MiniKV::Stop() {
    m_bStopped = true;
    hw_sw_memory_barrier();
    return true;
}

common::SP_PB_MSG MiniKV::Put(common::KVPutRequest req) {
    LOGDTAG;
    auto pbResp = new protocol::PutResponse();
    auto rs = common::SP_PB_MSG(pbResp);
    if (m_bStopped) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(ServiceIsStoppedError);

        return rs;
    }

    if (!req->has_entry() || req->entry().key().empty()) {
        pbResp->set_rc(protocol::Code::ArgsErr);
        pbResp->set_errmsg(EntryOrKeyIsEmptyError);
        return rs;
    }

    // acc check
    // TODO(sunchao): add req timeout config and open acc

    // write wal
    std::shared_ptr<protocol::Entry> pbRawEntry(req->release_entry());
    std::unique_ptr<WalPutEntry> pWalPutEntry(new WalPutEntry(m_pMp, pbRawEntry));
    wal::AppendEntryResult walRs;
    {
        std::unique_lock<std::mutex> l(m_walLock);
        walRs = m_pWal->AppendEntry(pWalPutEntry.get());
    }
    if (walRs.Rc != wal::Code::OK) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(std::move(walRs.Errmsg));

        return rs;
    }

    // insert into kv in memory
    MiniKVKey k = {
            .Data = reinterpret_cast<uchar*>(const_cast<char*>(pbRawEntry->key().c_str())),
            .Len = uint32_t(pbRawEntry->key().length())
    };

    auto pRawEntry = new RawPbEntryEntry(m_pMp, pbRawEntry);
    {
        sys::WriteLock wl(&m_kvLock);
        m_kvs[k] = pRawEntry;
    }

    pbResp->set_rc(protocol::Code::OK);
    return rs;
}

common::SP_PB_MSG MiniKV::Get(common::KVGetRequest req) {
    LOGDTAG;
    auto pbResp = new protocol::GetResponse();
    auto rs = common::SP_PB_MSG(pbResp);
    if (m_bStopped) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(ServiceIsStoppedError);
        return rs;
    }

    if (req->key().empty()) {
        pbResp->set_rc(protocol::Code::ArgsErr);
        pbResp->set_errmsg(KeyIsEmptyError);
        return rs;
    }

    // acc check
    // TODO(sunchao): add req timeout config and open acc

    MiniKVKey k = {
            .Data = reinterpret_cast<uchar*>(const_cast<char*>(req->key().c_str())),
            .Len = uint32_t(req->key().length())
    };

    pbResp->set_rc(protocol::Code::OK);
    protocol::Entry *prsEntry = nullptr;
    std::map<MiniKVKey, RawPbEntryEntry*>::iterator iter;
    {
        sys::ReadLock rl(&m_kvLock);
        iter = m_kvs.find(k);
        if (iter == m_kvs.end()) {
            return rs;
        }

        prsEntry = new protocol::Entry();
        prsEntry->CopyFrom(*iter->second->Get());
        pbResp->set_allocated_entry(prsEntry);
    }

    return rs;
}

common::SP_PB_MSG MiniKV::Delete(common::KVDeleteRequest req) {
    LOGDTAG;
    auto pbResp = new protocol::DeleteResponse();
    auto rs = common::SP_PB_MSG(pbResp);
    if (m_bStopped) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(ServiceIsStoppedError);
        return rs;
    }

    if (req->key().empty()) {
        pbResp->set_rc(protocol::Code::ArgsErr);
        pbResp->set_errmsg(KeyIsEmptyError);
        return rs;
    }

    // acc check
    // TODO(sunchao): add req timeout config and open acc

    // write wal
    std::unique_ptr<WalDeleteEntry> wde(new WalDeleteEntry(m_pMp, reinterpret_cast<uchar*>(const_cast<char*>(req->key().c_str())), req->key().length(), false));
    wal::AppendEntryResult walRs;
    {
        std::unique_lock<std::mutex> l(m_walLock);
        walRs = m_pWal->AppendEntry(wde.get());
    }
    if (walRs.Rc != wal::Code::OK) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(std::move(walRs.Errmsg));

        return rs;
    }

    pbResp->set_rc(protocol::Code::OK);
    MiniKVKey k = {
            .Data = reinterpret_cast<uchar*>(const_cast<char*>(req->key().c_str())),
            .Len = uint32_t(req->key().length())
    };

    {
        WriteLock wl(&m_kvLock);
        auto iter = m_kvs.find(k);
        if (iter != m_kvs.end()) {
            m_kvs.erase(iter);
        }
    }

    return rs;
}

common::SP_PB_MSG MiniKV::Scan(common::KVScanRequest req) {
    LOGDTAG;
    auto pbResp = new protocol::ScanResponse();
    auto rs = common::SP_PB_MSG(pbResp);
    if (m_bStopped) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(ServiceIsStoppedError);
        return rs;
    }

    if (0 == req->limit()) {
        pbResp->set_rc(protocol::Code::OK);
        return rs;
    }

    static const char *s_empty = "\0";
    const uchar *sk = reinterpret_cast<uchar*>(const_cast<char*>("\0"));
    if (!req->startkey().empty()) {
        sk = reinterpret_cast<uchar*>(const_cast<char*>(req->startkey().c_str()));
    }

    MiniKVKey k = {
            .Data = sk,
            .Len = uint32_t(req->startkey().length())
    };

    pbResp->set_rc(protocol::Code::OK);
    {
        ReadLock rl(&m_kvLock);
        auto iter = m_kvs.lower_bound(k);
        if (m_kvs.end() == iter) {
            return rs;
        }

        if (req->containstartkey()) {
            auto prsEntry = pbResp->add_entries();
            prsEntry->CopyFrom(*iter->second->Get().get());
        }

        if (req->order() == protocol::SortOrder::Asc) {
            ++iter;
            for (;iter != m_kvs.end(); ++iter) {
                auto prsEntry = pbResp->add_entries();
                prsEntry->CopyFrom(*iter->second->Get().get());
            }
        } else {
            --iter;
            for (;iter != m_kvs.end(); --iter) {
                auto prsEntry = pbResp->add_entries();
                prsEntry->CopyFrom(*iter->second->Get().get());
            }
        }
    }

    return rs;
}

common::IEntry *MiniKV::GetNextEntry() {
    return nullptr;
}

bool MiniKV::Empty() {
    return false;
}

uint64_t MiniKV::MaxId() {
    return 0;
}

common::IEntry* MiniKV::create_wal_new_entry(const common::Buffer &b) {
    auto type = EntryType(*b.GetPos());
    switch (type) {
    case EntryType::WalPut: {
        return new WalPutEntry(m_pMp);
    }
    case EntryType::WalDelete: {
        return new WalDeleteEntry(m_pMp);
    }
    default:
        return nullptr;
    }
}

common::IEntry* MiniKV::create_checkpoint_new_entry(const common::Buffer &b) {
    return new RawPbEntryEntry(m_pMp);
}

void MiniKV::on_checkpoint_load_entry(std::vector<common::IEntry*> entries) {

}

void MiniKV::on_wal_load_entries(std::vector<wal::WalEntry> entries) {

}
}
}
