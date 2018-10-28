/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "mini-kv.h"

namespace minikv {
namespace kv {
MiniKV::MiniKV(std::string &walType, std::string &checkpointType) {
    m_pMp = new sys::MemPool();
}

MiniKV::~MiniKV() {
    DELETE_PTR(m_pMp);
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
}
}