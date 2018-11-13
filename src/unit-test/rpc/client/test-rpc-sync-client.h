/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_TEST_RPC_SYNC_CLIENT_H
#define FLYINGKV_TEST_RPC_SYNC_CLIENT_H

#include "../../../rpc/abstract-rpc-sync-client.h"
#include "../../../rpc/common-def.h"

namespace flyingkv {
namespace protocol {
class AppendEntriesResponse;
class RequestVoteResponse;
}

namespace test {
class TestRpcClientSync : public rpc::ARpcSyncClient {
PUBLIC
    TestRpcClientSync(net::ISocketService *ss, const sys::cctime &timeout,
                                uint16_t workThreadsCnt, sys::MemPool *memPool = nullptr) :
        rpc::ARpcSyncClient(ss, timeout, memPool) {}

    // Define Rpc start
    DefineStandardSyncRpcWithMsgId(Get);
    // Define Rpc end

PROTECTED
    bool onStart() override;
    bool onStop() override;

PRIVATE
    bool register_rpc_handlers();
};
} // namespace test
} // namespace flyingkv

#endif //FLYINGKV_TEST_RPC_SYNC_CLIENT_H
