/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../common/ientry.h"
#include "../../wal/wal-factory.h"
#include "../../checkpoint/checkpoint-factory.h"

#include "entry.h"

#include "mini-kv.h"

namespace minikv {
namespace mnkv {
MiniKV::MiniKV(std::string &walType, std::string &checkpointType, std::string &walDir,
               std::string &checkpointDir, uint32_t maxPendingCnt) {
    m_pMp = new sys::MemPool();
    m_pWal = wal::WALFactory::CreateInstance(walType, walDir, std::bind(&MiniKV::create_new_entry, this));
    m_pCheckpoint = checkpoint::CheckpointFactory::CreateInstance(checkpointType, checkpointDir, std::bind(&MiniKV::create_new_entry, this));
    m_pbqPendingTasks = new common::BlockingQueue<common::SP_PB_MSG>(maxPendingCnt);
}

MiniKV::~MiniKV() {
    DELETE_PTR(m_pMp);
    DELETE_PTR(m_pWal);
    DELETE_PTR(m_pCheckpoint);
    DELETE_PTR(m_pbqPendingTasks);
}

bool MiniKV::Start() {
    return false;
}

bool MiniKV::Stop() {
    return false;
}

common::SP_PB_MSG MiniKV::OnPut(common::KVPutRequest req) {
    return common::SP_PB_MSG(nullptr);
}

common::SP_PB_MSG MiniKV::OnGet(common::KVGetRequest req) {
    return common::SP_PB_MSG(nullptr);
}

common::SP_PB_MSG MiniKV::OnDelete(common::KVDeleteRequest req) {
    return common::SP_PB_MSG(nullptr);
}

common::SP_PB_MSG MiniKV::OnScan(common::KVScanRequest req) {
    return common::SP_PB_MSG(nullptr);
}

common::IEntry* MiniKV::create_new_entry() {
    return new Entry(m_pMp);
}
}
}
