/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <iostream>
#include <gtest/gtest.h>

#include "../../../common/server-gflags-config.h"
#include "../../../rpc/rpc-handler.h"
#include "../../../common/rpc-def.h"
#include "../../../rpc/common-def.h"

#include "test-rpc-server.h"
#include "../../../codegen/kvrpc.pb.h"

namespace minikv {
namespace test {
TestRpcServer::TestRpcServer(uint16_t workThreadsCnt, net::ISocketService *ss, sys::MemPool *memPool) {
    m_pRpcServer = new rpc::RpcServer(workThreadsCnt, ss, memPool);
}

TestRpcServer::~TestRpcServer() {
    DELETE_PTR(m_pRpcServer);
}

bool TestRpcServer::Start() {
    register_rpc_handlers();

    return m_pRpcServer->Start();
}

bool TestRpcServer::Stop() {
    return m_pRpcServer->Stop();
}

void TestRpcServer::HandleMessage(std::shared_ptr<net::NotifyMessage> sspNM) {
    m_pRpcServer->HandleMessage(sspNM);
}

void TestRpcServer::register_rpc_handlers() {
    // internal communication
    auto getHandler = new rpc::TypicalRpcHandler(std::bind(&TestRpcServer::on_get, this, std::placeholders::_1),
                                                 std::bind(&TestRpcServer::create_get_request, this));
    m_pRpcServer->RegisterRpc(GET_RPC_ID, getHandler);

    m_pRpcServer->FinishRegisterRpc();
}

rpc::SP_PB_MSG TestRpcServer::on_get(rpc::SP_PB_MSG sspMsg) {
    auto getReq = dynamic_cast<protocol::GetRequest*>(sspMsg.get());
    auto k = getReq->key();
    EXPECT_EQ(k.c_str()[0], 'a');
    EXPECT_EQ(k.c_str()[1], 'b');
    EXPECT_EQ(k.c_str()[2], 'c');
    EXPECT_EQ(k.c_str()[3], 'd');

    auto response = new protocol::GetResponse();
    response->set_rc(minikv::protocol::OK);

    return rpc::SP_PB_MSG(response);
}

rpc::SP_PB_MSG TestRpcServer::create_get_request() {
    return rpc::SP_PB_MSG(new protocol::GetRequest());
}
}
}
