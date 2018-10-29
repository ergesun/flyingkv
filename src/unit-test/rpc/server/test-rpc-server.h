/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_TEST_RPC_SERVER_H
#define FLYINGKV_TEST_RPC_SERVER_H

#include "../../../common/iservice.h"
#include "../../../rpc/rpc-server.h"
#include "../../../rpc/common-def.h"
#include "../../../common/ikv-common.h"

namespace flyingkv {
namespace test {
class TestRpcServer : public common::IService, public rpc::IMessageHandler {
public:
    TestRpcServer(uint16_t workThreadsCnt, net::ISocketService *ss, sys::MemPool *memPool = nullptr);
    ~TestRpcServer() override;

    bool Start() override;
    bool Stop() override;

    void HandleMessage(std::shared_ptr<net::NotifyMessage> sspNM) override;

private:
    void register_rpc_handlers();
    common::SP_PB_MSG on_get(common::SP_PB_MSG sspMsg);
    common::SP_PB_MSG create_get_request();

private:
    rpc::RpcServer        *m_pRpcServer = nullptr;
};
} // namespace test
} // namespace flyingkv

#endif //FLYINGKV_TEST_RPC_SERVER_H
