/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <iostream>
#include <memory>
#include <gtest/gtest.h>

#include "../../net/isocket-service.h"
#include "../../common/global-vars.h"
#include "../../rpc/rpc-handler.h"
#include "../../rpc/common-def.h"
#include "../../net/rcv-message.h"
#include "../../common/buffer.h"
#include "../../net/net-protocol-stacks/inet-stack-worker-manager.h"
#include "../../net/socket-service-factory.h"
#include "../../net/net-protocol-stacks/msg-worker-managers/unique-worker-manager.h"
#include "../../codegen/node-raft.pb.h"
#include "../../server/node/rpc/rf-srv-rpc-sync-client.h"

#include "server/test-rpc-server.h"

using namespace std;

#define TEST_PORT            43225

minikv::net::ISocketService                 *g_pSS     = nullptr;
minikv::sys::MemPool                        *g_mp      = nullptr;
minikv::test::TestRpcServer                 *g_pServer = nullptr;
minikv::server::RfSrvInternalRpcClientSync  *g_pClient = nullptr;
volatile bool                                g_bStopped = false;

void dispatch_msg(std::shared_ptr<minikv::net::NotifyMessage> sspNM);

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    minikv::common::initialize();

    g_mp = new minikv::sys::MemPool();
    auto nat = new minikv::net::net_addr_t("0.0.0.0", TEST_PORT);
    std::shared_ptr<minikv::net::net_addr_t> sspNat(nat);
    timeval connTimeout = {
        .tv_sec = 0,
        .tv_usec = 100 * 1000
    };

    minikv::net::NssConfig nc = {
        .sp = minikv::net::SocketProtocol::Tcp,
        .sspNat = sspNat,
        .logicPort = TEST_PORT,
        .netMgrType = minikv::net::NetStackWorkerMgrType::Unique,
        .memPool = g_mp,
        .msgCallbackHandler = std::bind(&dispatch_msg, std::placeholders::_1),
        .connectTimeout = connTimeout
    };
    g_pSS = minikv::net::SocketServiceFactory::CreateService(nc);

    g_pSS->Start(2, minikv::net::NonBlockingEventModel::Posix);
    g_pServer = new minikv::test::TestRpcServer(1, g_pSS, g_mp);

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
    minikv::sys::cctime timeout(100, 1000000 * 200);
    // test sync rpc client
    g_pClient = new minikv::server::RfSrvInternalRpcClientSync(g_pSS, timeout, 1, g_mp);

    EXPECT_EQ(g_pClient->Start(), true);
    minikv::net::net_peer_info_t peer = {
        .nat = {
            .addr = "localhost",
            .port = TEST_PORT
        },
        .sp = minikv::net::SocketProtocol::Tcp
    };

    auto appendEntriesRequest = new minikv::protocol::AppendEntriesRequest();
    appendEntriesRequest->set_term(1234);
    appendEntriesRequest->set_leaderid(1);
    appendEntriesRequest->set_prevlogindex(22);
    appendEntriesRequest->set_prevlogterm(1233);
    appendEntriesRequest->add_entries();
    auto entry = appendEntriesRequest->mutable_entries(0);
    entry->set_term(1233);
    entry->set_index(25);
    entry->set_type(minikv::protocol::RfLogEntryType::CONFIGURATION);
    entry->set_data("test entry data!");

    auto tmpPeer = peer;
    std::shared_ptr<minikv::protocol::AppendEntriesResponse> appendEntriesSspResp;
    EXPECT_NO_THROW(appendEntriesSspResp = g_pClient->AppendEntries(minikv::rpc::SP_PB_MSG(appendEntriesRequest), std::move(tmpPeer)));
    EXPECT_EQ(appendEntriesSspResp->term(), 1111);
    EXPECT_EQ(appendEntriesSspResp->success(), true);
    std::cout << "server resp: term = " << appendEntriesSspResp->term() << ", ok = " << appendEntriesSspResp->success() << std::endl;

    auto reqVoteRequest = new minikv::protocol::RequestVoteRequest();
    reqVoteRequest->set_term(1234);
    reqVoteRequest->set_candidateid(1);
    reqVoteRequest->set_lastlogindex(22);
    reqVoteRequest->set_lastlogterm(1233);

    std::shared_ptr<minikv::protocol::RequestVoteResponse> reqVoteSspResp;
    EXPECT_NO_THROW(reqVoteSspResp = g_pClient->RequestVote(minikv::rpc::SP_PB_MSG(reqVoteRequest), std::move(tmpPeer = peer)));
    EXPECT_EQ(reqVoteSspResp->term(), 1111);
    EXPECT_EQ(reqVoteSspResp->votegranted(), true);
    std::cout << "server resp: term = " << reqVoteSspResp->term() << ", ok = " << reqVoteSspResp->votegranted() << std::endl;

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
                auto buf = rm->GetDataBuffer();
                auto messageType = (minikv::rpc::MessageType)(*(buf->GetPos()));
                buf->MoveHeadBack(1);
                if (minikv::rpc::MessageType::Request == messageType) {
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
