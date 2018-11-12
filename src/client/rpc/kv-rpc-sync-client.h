/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_RPC_SYNC_CLIENT_H
#define FLYINGKV_KV_RPC_SYNC_CLIENT_H

#include "../../rpc/abstract-rpc-sync-client.h"
#include "../../rpc/common-def.h"

namespace flyingkv {
namespace protocol {
class PutResponse;
class GetResponse;
class DeleteResponse;
class ScanResponse;
}

namespace client {
class KVRpcClientSync : public rpc::ARpcSyncClient {
PUBLIC
    KVRpcClientSync(net::ISocketService *ss, const sys::cctime &timeout, sys::MemPool *memPool = nullptr) :
        rpc::ARpcSyncClient(ss, timeout, memPool) {}

    // Define Rpc start
    DefineStandardSyncRpcWithNoMsgId(Put);
    DefineStandardSyncRpcWithNoMsgId(Get);
    DefineStandardSyncRpcWithNoMsgId(Delete);
    DefineStandardSyncRpcWithNoMsgId(Scan);
    //DefineStandardSyncRpcWithMsgId(Put);
    //DefineStandardSyncRpcWithMsgId(Get);
    // Define Rpc end

PROTECTED
    bool onStart() override;
    bool onStop() override;

PRIVATE
    bool register_rpc_handlers();
};
} // namespace client
} // namespace flyingkv

#endif //FLYINGKV_KV_RPC_SYNC_CLIENT_H
