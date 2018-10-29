/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <iostream>
#include "../test-snd-message.h"

#include "../../../../sys/mem-pool.h"
#include "../../../../common/buffer.h"
#include "../../../../net/socket-service-factory.h"
#include "../../../../net/rcv-message.h"
#include "../../../../net/net-protocol-stacks/msg-worker-managers/unique-worker-manager.h"

#include "tcp-server-test-case.h"

namespace flyingkv {
namespace test {
net::ISocketService     *TcpServerTest::s_ss = nullptr;
sys::MemPool            *TcpServerTest::m_mp = nullptr;

void TcpServerTest::Run() {
    auto nat = new flyingkv::net::net_addr_t("0.0.0.0", 2210);
    std::shared_ptr<flyingkv::net::net_addr_t> ssp_npt(nat);
    m_mp = new flyingkv::sys::MemPool();
    timeval connTimeout = {
        .tv_sec = 0,
        .tv_usec = 100 * 1000
    };
    net::NssConfig nc = {
        .sp = flyingkv::net::SocketProtocol::Tcp,
        .sspNat = ssp_npt,
        .logicPort = 2210,
        .netMgrType = flyingkv::net::NetStackWorkerMgrType::Unique,
        .memPool = m_mp,
        .msgCallbackHandler = std::bind(&TcpServerTest::recv_msg, std::placeholders::_1),
        .connectTimeout = connTimeout
    };
    s_ss = net::SocketServiceFactory::CreateService(nc);
    if (!s_ss->Start(2, flyingkv::net::NonBlockingEventModel::Posix)) {
        throw std::runtime_error("cannot start SocketService");
    }
}

void TcpServerTest::recv_msg(std::shared_ptr<flyingkv::net::NotifyMessage> sspNM) {
    switch (sspNM->GetType()) {
        case flyingkv::net::NotifyMessageType::Message: {
            flyingkv::net::MessageNotifyMessage *mnm = dynamic_cast<flyingkv::net::MessageNotifyMessage*>(sspNM.get());
            auto rm = mnm->GetContent();
            if (rm) {
                auto respBuf = rm->GetDataBuffer();
#ifdef WITH_MSG_ID
                #ifdef BULK_MSG_ID
                std::cout << "request = "  << respBuf->GetPos() << ", " << "message id is { ts = " << rm->GetId().ts
                          << ", seq = " << rm->GetId().seq << "}" << std::endl;
#else
                std::cout << "request = "  << respBuf->GetPos() << ", " << "message id is " << rm->GetId() << "." << std::endl;
#endif
                TestSndMessage *tsm = new TestSndMessage(m_mp, rm->GetPeerInfo(),  rm->GetId(), "server response: hello client!");
#else
                std::cout << "request = "  << respBuf->GetPos() << "." << std::endl;
                std::stringstream ss;
                ss << "Server response: hello client! Req idx = " << atomic_addone_and_fetch(&s_idx) << ".";
                TestSndMessage *tsm = new TestSndMessage(m_mp, rm->GetPeerInfo(), ss.str());
#endif
                bool rc = s_ss->SendMessage(tsm);
                if (rc) {
                }
            }
            break;
        }
        case flyingkv::net::NotifyMessageType::Worker : {
            auto *wnm = dynamic_cast<flyingkv::net::WorkerNotifyMessage*>(sspNM.get());
            if (wnm) {
                std::cout << "worker notify message , rc = " << (int)wnm->GetCode() << ", message = " << wnm->What() << std::endl;
            }
            break;
        }
        case flyingkv::net::NotifyMessageType::Server: {
            auto *snm = dynamic_cast<flyingkv::net::ServerNotifyMessage*>(sspNM.get());
            if (snm) {
                std::cout << "server notify message , rc = " << (int)snm->GetCode() << ", message = " << snm->What() << std::endl;
            }
            break;
        }
    }
}
}
}
