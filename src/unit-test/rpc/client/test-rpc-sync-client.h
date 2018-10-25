/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_TEST_RPC_SYNC_CLIENT_H
#define MINIKV_TEST_RPC_SYNC_CLIENT_H

#include "../../../rpc/abstract-rpc-sync-client.h"
#include "../../../rpc/common-def.h"

namespace minikv {
namespace protocol {
class AppendEntriesResponse;
class RequestVoteResponse;
}

namespace test {
class TestRpcClientSync : public rpc::ARpcSyncClient {
public:
    TestRpcClientSync(net::ISocketService *ss, const sys::cctime &timeout,
                                uint16_t workThreadsCnt, sys::MemPool *memPool = nullptr) :
        rpc::ARpcSyncClient(ss, timeout, workThreadsCnt, memPool) {}

    // Define Rpc start
    DefineStandardSyncRpcWithMsgId(Get);
    // Define Rpc end

protected:
    bool onStart() override;
    bool onStop() override;

private:
    bool register_rpc_handlers();
};
} // namespace test
} // namespace minikv

#endif //MINIKV_TEST_RPC_SYNC_CLIENT_H
