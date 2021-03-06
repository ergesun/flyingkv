/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../common/server-gflags-config.h"
#include "../../common/rpc-def.h"
#include "../../rpc/rpc-handler.h"
#include "../../codegen/kvrpc.pb.h"
#include "../../net/net-protocol-stacks/nonblocking/nss-config.h"
#include "../../net/socket-service-factory.h"
#include "../../net/rcv-message.h"
#include "../../sys/gcc-buildin.h"
#include "../../net/notify-message.h"

#include "../../common/ikv-common.h"

#include "kv-rpc-sync-server.h"

namespace flyingkv {
namespace server {
KVRpcServerSync::KVRpcServerSync(common::IKVOperator *handler, uint16_t workThreadsCnt, uint16_t netIOThreadsCnt,
                                 uint16_t port, sys::MemPool *memPool) : m_pHandler(handler) {

    CHECK(handler);
    m_pMemPool = memPool;
    if (!memPool) {
        m_bOwnMemPool = true;
        // TODO(sunchao): 优化此内存池参数。
        m_pMemPool = new sys::MemPool();
    }

    m_iIOThreadsCnt = netIOThreadsCnt;
    auto nat = new net::net_addr_t("0.0.0.0", port);
    std::shared_ptr<net::net_addr_t> sspNat(nat);
    timeval connTimeout = {
            .tv_sec = 0,
            .tv_usec = 0
    };

    net::NssConfig nc(net::SocketProtocol::Tcp, sspNat, port, net::NetStackWorkerMgrType::Unique,
                      m_pMemPool, std::bind(&KVRpcServerSync::on_recv_net_message, this, std::placeholders::_1),
                      connTimeout);
    m_pSocketService = net::SocketServiceFactory::CreateService(nc);
    m_pRpcServer = new rpc::RpcServerSync(workThreadsCnt, m_pSocketService, memPool);
}

KVRpcServerSync::~KVRpcServerSync() {
    DELETE_PTR(m_pRpcServer);
    DELETE_PTR(m_pSocketService);
    if (m_bOwnMemPool) {
        DELETE_PTR(m_pMemPool);
    }
}

bool KVRpcServerSync::Start() {
    if (!m_bStopped) {
        return true;
    }

    m_bStopped = false;
    hw_sw_memory_barrier();
    register_rpc_handlers();

    if (!m_pRpcServer->Start()) {
        return false;
    }

    return m_pSocketService->Start(m_iIOThreadsCnt, net::NonBlockingEventModel::Posix);
}

bool KVRpcServerSync::Stop() {
    if (m_bStopped) {
        return true;
    }

    m_bStopped = true;
    hw_sw_memory_barrier();

    return m_pRpcServer->Stop();
}

void KVRpcServerSync::register_rpc_handlers() {
    // internal communication
    auto putHandler = new rpc::TypicalRpcHandler(std::bind(&KVRpcServerSync::on_put, this, std::placeholders::_1),
                                              std::bind(&KVRpcServerSync::create_put_request, this));
    m_pRpcServer->RegisterRpc(PUT_RPC_ID, putHandler);
    auto getHandler = new rpc::TypicalRpcHandler(std::bind(&KVRpcServerSync::on_get, this, std::placeholders::_1),
                                              std::bind(&KVRpcServerSync::create_get_request, this));
    m_pRpcServer->RegisterRpc(GET_RPC_ID, getHandler);
    auto deleteHandler = new rpc::TypicalRpcHandler(std::bind(&KVRpcServerSync::on_delete, this, std::placeholders::_1),
                                                 std::bind(&KVRpcServerSync::create_delete_request, this));
    m_pRpcServer->RegisterRpc(DELETE_RPC_ID, deleteHandler);
    auto scanHandler = new rpc::TypicalRpcHandler(std::bind(&KVRpcServerSync::on_scan, this, std::placeholders::_1),
                                                 std::bind(&KVRpcServerSync::create_scan_request, this));
    m_pRpcServer->RegisterRpc(SCAN_RPC_ID, scanHandler);

    m_pRpcServer->FinishRegisterRpc();
}

common::SP_PB_MSG KVRpcServerSync::on_put(common::SP_PB_MSG sspMsg) {
    return m_pHandler->Put(sspMsg);
}

common::SP_PB_MSG KVRpcServerSync::create_put_request() {
    return common::SP_PB_MSG(new protocol::PutRequest());
}

common::SP_PB_MSG KVRpcServerSync::on_get(common::SP_PB_MSG sspMsg) {
    return m_pHandler->Get(sspMsg);
}

common::SP_PB_MSG KVRpcServerSync::create_get_request() {
    return common::SP_PB_MSG(new protocol::GetRequest());
}

common::SP_PB_MSG KVRpcServerSync::on_delete(common::SP_PB_MSG sspMsg) {
    return m_pHandler->Delete(sspMsg);
}

common::SP_PB_MSG KVRpcServerSync::create_delete_request() {
    return common::SP_PB_MSG(new protocol::DeleteRequest());
}

common::SP_PB_MSG KVRpcServerSync::on_scan(common::SP_PB_MSG sspMsg) {
    return m_pHandler->Scan(sspMsg);
}

common::SP_PB_MSG KVRpcServerSync::create_scan_request() {
    return common::SP_PB_MSG(new protocol::ScanRequest());
}

void KVRpcServerSync::on_recv_net_message(std::shared_ptr<net::NotifyMessage> sspNM) {
    if (UNLIKELY(m_bStopped)) {
        return;
    }

    m_pRpcServer->HandleMessage(std::move(sspNM));
}
} // namespace server
} // namespace flyingkv
