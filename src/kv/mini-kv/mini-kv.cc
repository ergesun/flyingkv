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
#include "../../common/global-vars.h"
#include "mini-kv-traveller.h"

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
    m_triggerCheckWalCallback = std::bind(&MiniKV::trigger_check_wal, this, std::placeholders::_1);
    m_triggerCheckWalEvent = Timer::Event(nullptr, &m_triggerCheckWalCallback);
    m_triggerCheckWalSizeTickSeconds = pc->CheckWalSizeTickSeconds;
    m_triggerCheckpointWalSizeByte = pc->DoCheckpointWalSizeBytes;
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

    m_applyId = cpLoadRs.MaxId;
    auto walInitRs = m_pWal->Init();
    if (wal::Code::OK != walInitRs.Rc) {
        return false;
    }

    auto walLoadRs = m_pWal->Load(std::bind(&MiniKV::on_wal_load_entries, this, std::placeholders::_1));
    auto ok = wal::Code::OK == walLoadRs.Rc;
    if (ok) {
        m_bStopped = false;
        hw_sw_memory_barrier();
        m_pCheckerThread = new std::thread([&](){
            while (!m_bStopped) {
                while (!m_bCheck) {
                    std::unique_lock<std::mutex> l(m_checkMtx);
                    m_checkCV.wait(l);
                }

                check_wal();
            }
        });

        register_trigger_check_wal_timer();
    }

    return ok;
}

bool MiniKV::Stop() {
    m_bStopped = true;
    hw_sw_memory_barrier();
    return true;
}

common::SP_PB_MSG MiniKV::Put(common::SP_PB_MSG rawReq) {
    LOGDTAG;
    auto req = dynamic_cast<protocol::PutRequest*>(rawReq.get());
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
    std::unique_lock<std::mutex> l(m_walLock);
    auto walRs = m_pWal->AppendEntry(pWalPutEntry.get());
    if (walRs.Rc != wal::Code::OK) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(std::move(walRs.Errmsg));

        return rs;
    }

    if (m_applyId < walRs.EntryId) {
        m_applyId = walRs.EntryId;
    }

    // insert into kv in memory
    Key k = {
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

common::SP_PB_MSG MiniKV::Get(common::SP_PB_MSG rawReq) {
    LOGDTAG;
    auto req = dynamic_cast<protocol::GetRequest*>(rawReq.get());
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

    Key k = {
            .Data = reinterpret_cast<uchar*>(const_cast<char*>(req->key().c_str())),
            .Len = uint32_t(req->key().length())
    };

    pbResp->set_rc(protocol::Code::OK);
    protocol::Entry *prsEntry = nullptr;
    std::map<Key, RawPbEntryEntry*>::iterator iter;
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

common::SP_PB_MSG MiniKV::Delete(common::SP_PB_MSG rawReq) {
    LOGDTAG;
    auto req = dynamic_cast<protocol::DeleteRequest*>(rawReq.get());
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
    std::unique_lock<std::mutex> l(m_walLock);
    auto walRs = m_pWal->AppendEntry(wde.get());
    if (walRs.Rc != wal::Code::OK) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(std::move(walRs.Errmsg));

        return rs;
    }

    if (m_applyId < walRs.EntryId) {
        m_applyId = walRs.EntryId;
    }

    pbResp->set_rc(protocol::Code::OK);
    Key k = {
            .Data = reinterpret_cast<uchar*>(const_cast<char*>(req->key().c_str())),
            .Len = uint32_t(req->key().length())
    };

    {
        WriteLock wl(&m_kvLock);
        auto iter = m_kvs.find(k);
        if (iter != m_kvs.end()) {
            auto tmp =  iter->second;
            m_kvs.erase(iter);
            delete tmp;
        }
    }

    return rs;
}

common::SP_PB_MSG MiniKV::Scan(common::SP_PB_MSG rawReq) {
    LOGDTAG;
    auto req = dynamic_cast<protocol::ScanRequest*>(rawReq.get());
    auto pbResp = new protocol::ScanResponse();
    auto rs = common::SP_PB_MSG(pbResp);
    if (m_bStopped) {
        pbResp->set_rc(protocol::Code::InternalErr);
        pbResp->set_errmsg(ServiceIsStoppedError);
        return rs;
    }

    auto limit = req->limit();
    if (0 == limit) {
        pbResp->set_rc(protocol::Code::OK);
        return rs;
    }

    static const char *s_empty = "\0";
    const uchar *sk = reinterpret_cast<uchar*>(const_cast<char*>("\0"));
    if (!req->startkey().empty()) {
        sk = reinterpret_cast<uchar*>(const_cast<char*>(req->startkey().c_str()));
    }

    Key k = {
            .Data = sk,
            .Len = uint32_t(req->startkey().length())
    };

    uint32_t curCnt = 0;
    pbResp->set_rc(protocol::Code::OK);
    {
        ReadLock rl(&m_kvLock);
        auto iter = m_kvs.lower_bound(k);
        if (m_kvs.end() == iter) {
            return rs;
        }

        if (req->containstartkey()) {
            ++curCnt;
            auto prsEntry = pbResp->add_entries();
            prsEntry->CopyFrom(*iter->second->Get().get());
        }

        if (req->order() == protocol::SortOrder::Asc) {
            ++iter;
            for (;(curCnt < limit) && (iter != m_kvs.end()); ++iter) {
                ++curCnt;
                auto prsEntry = pbResp->add_entries();
                prsEntry->CopyFrom(*iter->second->Get().get());
            }
        } else {
            --iter;
            for (;(curCnt < limit) && (iter != m_kvs.end()); --iter) {
                ++curCnt;
                auto prsEntry = pbResp->add_entries();
                prsEntry->CopyFrom(*iter->second->Get().get());
            }
        }
    }

    return rs;
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
    if (entries.empty()) {
        return;
    }

    for (auto p : entries) {
        auto entry = dynamic_cast<RawPbEntryEntry*>(p);
        Key k = {.Data = reinterpret_cast<uchar*>(const_cast<char*>(entry->Get()->key().c_str())),
                 .Len = uint32_t(entry->Get()->key().length())};
        m_kvs[k] = entry;
    }
}

void MiniKV::on_wal_load_entries(std::vector<wal::WalEntry> entries) {
    if (entries.empty()) {
        return;
    }

    for (auto p : entries) {
        if (m_applyId > p.Id) {
            continue;
        }

        m_applyId = p.Id;
        switch (EntryType(p.Entry->TypeId())) {
            case EntryType::WalDelete: {
                auto deleteEntry = dynamic_cast<WalDeleteEntry*>(p.Entry);
                auto kStr = deleteEntry->Get();
                Key k = {.Data = kStr,
                         .Len = uint32_t(strlen(reinterpret_cast<const char*>(kStr)))};
                m_kvs.erase(k);
                break;
            }
            case EntryType::WalPut: {
                auto putEntry = dynamic_cast<WalPutEntry*>(p.Entry);
                auto ssppbEntry = putEntry->Get();
                delete putEntry;
                Key k = {.Data = reinterpret_cast<uchar*>(const_cast<char*>(ssppbEntry->key().c_str())),
                        .Len = uint32_t(ssppbEntry->key().length())};
                m_kvs[k] = new RawPbEntryEntry(m_pMp, ssppbEntry);
                break;
            }
            default:
                LOGFFUN << "unknown entry type";
        }
    }
}

void MiniKV::register_trigger_check_wal_timer() {
    common::g_pTimer->SubscribeEventAfter(cctime(m_triggerCheckWalSizeTickSeconds, 0), m_triggerCheckWalEvent);
}

void MiniKV::trigger_check_wal(void *) {
    m_bCheck = true;
    m_checkCV.notify_one();
}

void MiniKV::check_wal() {
    m_bCheck = false;
    if (m_bStopped) {
        return;
    }

    auto walRs = m_pWal->Size();
    if (wal::Code::OK != walRs.Rc) {
        LOGFFUN << "wal error " << walRs.Errmsg;
    }
    if (walRs.Size >= m_triggerCheckpointWalSizeByte) {
        auto traveller = new MiniKVTraveller(&m_kvs, m_applyId, std::bind(&MiniKV::on_checkpoint_prepare, this),
                                             std::bind(&MiniKV::on_checkpoint_complete_prepare, this));
        auto cpRs = m_pCheckpoint->Save(traveller);
        delete traveller;
        if (checkpoint::Code::OK == cpRs.Rc) {
            std::unique_lock<std::mutex> l(m_walLock);
            LOGIFUN << "truncate wal to idx " << cpRs.MaxId;
            auto walTruncRs = m_pWal->Truncate(cpRs.MaxId);
            if (walTruncRs.Rc != wal::Code::OK) {
                LOGFFUN << "wal do truncate error " << walTruncRs.Errmsg;
            }
        } else {
            LOGEFUN << "save checkpoint error " << cpRs.Errmsg;
        }
    }

    register_trigger_check_wal_timer();
}

void MiniKV::on_checkpoint_prepare() {
    m_walLock.lock();
}

void MiniKV::on_checkpoint_complete_prepare() {
    m_walLock.unlock();
}
}
}
