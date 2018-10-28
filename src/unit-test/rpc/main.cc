/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <iostream>
#include <memory>
#include <gtest/gtest.h>

#include "../../net/isocket-service.h"
#include "../../common/global-vars.h"
#include "../../common/ikv-common.h"
#include "../../rpc/rpc-handler.h"
#include "../../rpc/common-def.h"
#include "../../net/rcv-message.h"
#include "../../common/buffer.h"
#include "../../net/net-protocol-stacks/inet-stack-worker-manager.h"
#include "../../net/socket-service-factory.h"
#include "../../net/net-protocol-stacks/msg-worker-managers/unique-worker-manager.h"
#include "../../codegen/kvrpc.pb.h"

#include "server/test-rpc-server.h"
#include "client/test-rpc-sync-client.h"

using namespace std;
using namespace minikv;

#define TEST_PORT            43225

net::ISocketService                 *g_pSS     = nullptr;
sys::MemPool                        *g_mp      = nullptr;
test::TestRpcServer                 *g_pServer = nullptr;
test::TestRpcClientSync             *g_pClient = nullptr;
volatile bool                        g_bStopped = false;

std::unordered_map<net::Message::Id, bool> g_sndMsgIds;

void dispatch_msg(std::shared_ptr<net::NotifyMessage> sspNM);

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    common::initialize();

    g_mp = new sys::MemPool();
    auto nat = new net::net_addr_t("0.0.0.0", TEST_PORT);
    std::shared_ptr<net::net_addr_t> sspNat(nat);
    timeval connTimeout = {
        .tv_sec = 0,
        .tv_usec = 100 * 1000
    };

    net::NssConfig nc = {
        .sp = minikv::net::SocketProtocol::Tcp,
        .sspNat = sspNat,
        .logicPort = TEST_PORT,
        .netMgrType = net::NetStackWorkerMgrType::Unique,
        .memPool = g_mp,
        .msgCallbackHandler = std::bind(&dispatch_msg, std::placeholders::_1),
        .connectTimeout = connTimeout
    };
    g_pSS = net::SocketServiceFactory::CreateService(nc);

    g_pSS->Start(2, net::NonBlockingEventModel::Posix);
    g_pServer = new test::TestRpcServer(1, g_pSS, g_mp);

    EXPECT_EQ(g_pServer->Start(), true);

    auto res = RUN_ALL_TESTS();
    sleep(1);
    g_bStopped = true;
    EXPECT_EQ(g_pServer->Stop(), true);
    DELETE_PTR(g_pServer);
    DELETE_PTR(g_pClient);
    EXPECT_EQ(g_pSS->Stop(), true);
    DELETE_PTR(g_pSS);
    DELETE_PTR(g_mp);

    return res;
}

TEST(RpcTest, ClientServerTest) {
    sys::cctime timeout(100, 1000000 * 200);
    // test sync rpc client
    g_pClient = new test::TestRpcClientSync(g_pSS, timeout, 1, g_mp);

    EXPECT_EQ(g_pClient->Start(), true);
    net::net_peer_info_t peer = {
        .nat = {
            .addr = "localhost",
            .port = TEST_PORT
        },
        .sp = net::SocketProtocol::Tcp
    };

    auto getReq = new protocol::GetRequest();
    getReq->set_key("abcd");
    auto tmpPeer = peer;
    std::shared_ptr<protocol::GetResponse> getRes;
    auto id = net::SndMessage::GetNewId();
    g_sndMsgIds[id] = false;
    EXPECT_NO_THROW(getRes = g_pClient->Get(id, common::SP_PB_MSG(getReq), std::move(tmpPeer)));

    std::cout << "server resp: rc = " << getRes->rc() << std::endl;

    EXPECT_EQ(g_pClient->Stop(), true);
}

void dispatch_msg(std::shared_ptr<minikv::net::NotifyMessage> sspNM) {
    if (g_bStopped) {
        return;
    }

    switch (sspNM->GetType()) {
        case minikv::net::NotifyMessageType::Message: {
            auto *mnm = dynamic_cast<minikv::net::MessageNotifyMessage*>(sspNM.get());
            auto rm = mnm->GetContent();
            if (LIKELY(rm)) {
                auto id = rm->GetId();
                if (!g_sndMsgIds[id]) {
                    g_sndMsgIds[id] = true;
                    g_pServer->HandleMessage(sspNM);
                    std::cout << "dispatch message type = Request." << std::endl;
                } else {
                    g_pClient->HandleMessage(sspNM);
                    std::cout << "dispatch message type = Response." << std::endl;
                }
            } else {
                LOGWFUN << "recv message is empty!";
            }
            break;
        }
        case minikv::net::NotifyMessageType::Worker: {
            g_pClient->HandleMessage(sspNM);
            g_pServer->HandleMessage(sspNM);
            break;
        }
        case minikv::net::NotifyMessageType::Server: {
            std::cerr << "Messenger port = " << TEST_PORT << " cannot start to work." << std::endl;
        }
    }
}
