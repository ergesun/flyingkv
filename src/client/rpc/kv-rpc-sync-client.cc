/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../net/notify-message.h"
#include "../../net/rcv-message.h"
#include "../../utils/protobuf-utils.h"
#include "../../common/buffer.h"
#include "../../codegen/kvrpc.pb.h"
#include "../../rpc/response.h"
#include "../../rpc/exceptions.h"
#include "../../common/rpc-def.h"

#include "kv-rpc-sync-client.h"

namespace flyingkv {
namespace client {
ImplStandardSyncRpcWithNoMsgId(KVRpcClientSync, Put)
ImplStandardSyncRpcWithNoMsgId(KVRpcClientSync, Get)
ImplStandardSyncRpcWithNoMsgId(KVRpcClientSync, Delete)
ImplStandardSyncRpcWithNoMsgId(KVRpcClientSync, Scan)

bool KVRpcClientSync::register_rpc_handlers() {
    if (!registerRpc(RpcPut, PUT_RPC_ID)) {
        return false;
    }

    if (!registerRpc(RpcGet, GET_RPC_ID)) {
        return false;
    }

    if (!registerRpc(RpcDelete, DELETE_RPC_ID)) {
        return false;
    }

    if (!registerRpc(RpcScan, SCAN_RPC_ID)) {
        return false;
    }

    finishRegisterRpc();

    return true;
}

bool KVRpcClientSync::onStart() {
    return register_rpc_handlers();
}

bool KVRpcClientSync::onStop() {
    return true;
}
} // namespace client
} // namespace flyingkv
