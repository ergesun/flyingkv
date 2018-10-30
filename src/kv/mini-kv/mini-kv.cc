/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../common/ientry.h"
#include "../../wal/wal-factory.h"
#include "../../checkpoint/checkpoint-factory.h"
#include "../../acc/config.h"

#include "entry.h"

#include "mini-kv.h"
#include "../../acc/resource-limit-control/simple-rlc.h"

namespace flyingkv {
namespace kv {
MiniKV::MiniKV(std::string &walType, std::string &walDir, std::string &checkpointType,
               std::string &checkpointDir, std::string &accConfPath, std::unordered_map<common::ReqRespType, int64_t> &reqTimeout) {
    m_hmReqTimeoutMs = reqTimeout;
    m_pMp = new sys::MemPool();
    m_pWal = wal::WALFactory::CreateInstance(walType, walDir, std::bind(&MiniKV::create_new_entry, this));
    m_pCheckpoint = checkpoint::CheckpointFactory::CreateInstance(checkpointType, checkpointDir, std::bind(&MiniKV::create_new_entry, this));
    auto accConf = acc::ParseAccConfig(accConfPath);
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
    if (!m_pCheckpoint->Load(std::bind(&MiniKV::on_checkpoint_load_entry, this, std::placeholders::_1))) {
        LOGFFUN << "load checkpoint failed!";
    }

    auto entries = m_pWal->Load();


    return false;
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

common::IEntry* MiniKV::create_new_entry() {
    return new Entry(m_pMp);
}

void MiniKV::on_checkpoint_load_entry(common::IEntry *entry) {

}

void MiniKV::on_wal_load_entries(std::vector<wal::WalEntry> &entries) {

}
}
}
