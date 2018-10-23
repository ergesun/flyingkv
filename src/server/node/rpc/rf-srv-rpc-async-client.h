/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_RF_NODE_RPC_ASYNC_CLIENT_H
#define MINIKV_RF_NODE_RPC_ASYNC_CLIENT_H

#include "../../../rpc/common-def.h"
#include "../../../rpc/abstract-rpc-client.h"

namespace minikv {
namespace protocol {
class AppendEntriesResponse;
class RequestVoteResponse;
}

namespace server {
class RfSrvInternalRpcClientAsync : public rpc::ARpcClient {
public:
    RfSrvInternalRpcClientAsync(net::ISocketService *ss, net::NotifyMessageCallbackHandler cb,
                                sys::MemPool *memPool = nullptr) : rpc::ARpcClient(ss, memPool) {
        m_fCallback = cb;
    }

    // Define Rpc start
    DefineStandardAsyncRpcWithNoMsgId(AppendEntries);
    DefineStandardAsyncRpcWithNoMsgId(RequestVote);
    DefineStandardAsyncRpcWithMsgId(AppendEntries);
    DefineStandardAsyncRpcWithMsgId(RequestVote);
    // Define Rpc end

protected:
    bool onStart() override;
    bool onStop() override;
    void onRecvMessage(std::shared_ptr<net::NotifyMessage> sspNM) override;

private:
    bool register_rpc_handlers();

private:
    net::NotifyMessageCallbackHandler        m_fCallback;
};
} // namespace server
} // namespace minikv

#endif //MINIKV_RF_NODE_RPC_ASYNC_CLIENT_H
