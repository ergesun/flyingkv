/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../common/ientry.h"
#include "../../wal/wal-factory.h"
#include "../../checkpoint/checkpoint-factory.h"
#include "../../acc/config.h"
#include "../../acc/resource-limit-control/simple-rlc.h"
#include "../../checkpoint/config.h"
#include "../../wal/config.h"

#include "../config.h"
#include "entry.h"
#include "mini-kv.h"

namespace flyingkv {
namespace kv {
MiniKV::MiniKV(const KVConfig *pc) {
    m_pMp = new sys::MemPool();
    auto pWalConf = new wal::LogCleanWalConfig(pc->WalType, pc->WalRootDirPath, std::bind(&MiniKV::create_new_entry, this),
                    pc->WalWriteEntryVersion, pc->WalMaxSegmentSize, pc->WalReadBatchSize);
    m_pWal = wal::WALFactory::CreateInstance(pWalConf);
    delete pWalConf;
    auto pCheckpointConf = new checkpoint::EntryOrderCheckpointConfig(pc->CheckpointType, pc->CheckpointRootDirPath,
                                                                      std::bind(&MiniKV::create_new_entry, this),
                                                pc->CheckpointWriteEntryVersion, pc->CheckpointReadBatchSize);
    m_pCheckpoint = checkpoint::CheckpointFactory::CreateInstance(pCheckpointConf);
    delete pCheckpointConf;
    auto accConf = acc::ParseAccConfig(pc->AccConfPath);
    auto rlc = new acc::SimpleRlc();
    if (!rlc->Init(accConf)) {
        LOGFFUN << "acc init failed.";
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

    return wal::Code::OK == walLoadRs.Rc;
}

bool MiniKV::Stop() {
    return false;
}

common::SP_PB_MSG MiniKV::Put(common::KVPutRequest req) {

    return common::SP_PB_MSG(nullptr);
}

common::SP_PB_MSG MiniKV::Get(common::KVGetRequest req) {
    return common::SP_PB_MSG(nullptr);
}

common::SP_PB_MSG MiniKV::Delete(common::KVDeleteRequest req) {
    return common::SP_PB_MSG(nullptr);
}

common::SP_PB_MSG MiniKV::Scan(common::KVScanRequest req) {
    return common::SP_PB_MSG(nullptr);
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

common::IEntry* MiniKV::create_new_entry() {
    return new Entry(m_pMp);
}

void MiniKV::on_checkpoint_load_entry(std::vector<common::IEntry*> entries) {

}

void MiniKV::on_wal_load_entries(std::vector<wal::WalEntry> entries) {

}
}
}
