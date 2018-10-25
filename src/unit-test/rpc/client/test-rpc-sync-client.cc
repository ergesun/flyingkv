/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../../net/notify-message.h"
#include "../../../net/rcv-message.h"
#include "../../../utils/protobuf-utils.h"
#include "../../../common/buffer.h"
#include "../../../rpc/response.h"
#include "../../../codegen/kvrpc.pb.h"

#include "test-rpc-sync-client.h"
#include "../../../common/rpc-def.h"

namespace minikv {
namespace test {
ImplStandardSyncRpcWithMsgId(TestRpcClientSync, Get)

bool TestRpcClientSync::register_rpc_handlers() {
    if (!registerRpc(RpcGet, GET_RPC_ID)) {
        return false;
    }

    finishRegisterRpc();

    return true;
}

bool TestRpcClientSync::onStart() {
    return register_rpc_handlers();
}

bool TestRpcClientSync::onStop() {
    return true;
}
} // namespace test
} // namespace minikv
