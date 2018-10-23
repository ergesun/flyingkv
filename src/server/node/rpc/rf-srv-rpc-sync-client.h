/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_RF_NODE_RPC_SYNC_CLIENT_H
#define MINIKV_RF_NODE_RPC_SYNC_CLIENT_H

#include "../../../rpc/abstract-rpc-sync-client.h"
#include "../../../rpc/common-def.h"

namespace minikv {
namespace protocol {
class AppendEntriesResponse;
class RequestVoteResponse;
}

namespace server {
class RfSrvInternalRpcClientSync : public rpc::ARpcSyncClient {
public:
    RfSrvInternalRpcClientSync(net::ISocketService *ss, const sys::cctime &timeout,
                                uint16_t workThreadsCnt, sys::MemPool *memPool = nullptr) :
        rpc::ARpcSyncClient(ss, timeout, workThreadsCnt, memPool) {}

    // Define Rpc start
    DefineStandardSyncRpcWithNoMsgId(AppendEntries);
    DefineStandardSyncRpcWithNoMsgId(RequestVote);
    DefineStandardSyncRpcWithMsgId(AppendEntries);
    DefineStandardSyncRpcWithMsgId(RequestVote);
    // Define Rpc end

protected:
    bool onStart() override;
    bool onStop() override;

private:
    bool register_rpc_handlers();
};
} // namespace server
} // namespace minikv

#endif //MINIKV_RF_NODE_RPC_SYNC_CLIENT_H
