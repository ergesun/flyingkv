/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "mini-kv.h"

namespace minikv {
namespace kv {
MiniKV::MiniKV() {
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

rpc::SP_PB_MSG MiniKV::OnPut(rpc::SP_PB_MSG sspMsg) {
    return rpc::SP_PB_MSG(nullptr);
}

rpc::SP_PB_MSG MiniKV::OnGet(rpc::SP_PB_MSG sspMsg) {
    return rpc::SP_PB_MSG(nullptr);
}

rpc::SP_PB_MSG MiniKV::OnDelete(rpc::SP_PB_MSG sspMsg) {
    return rpc::SP_PB_MSG(nullptr);
}

rpc::SP_PB_MSG MiniKV::OnScan(rpc::SP_PB_MSG sspMsg) {
    return rpc::SP_PB_MSG(nullptr);
}
}
}