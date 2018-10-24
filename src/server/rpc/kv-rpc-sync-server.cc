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

#include "../../kv/ikv-handler.h"

#include "kv-rpc-sync-server.h"

namespace minikv {
namespace server {
KVRpcServerSync::KVRpcServerSync(kv::IKVHandler *handler, uint16_t workThreadsCnt, uint16_t netIOThreadsCnt, uint16_t port,
                                 uint32_t connectTimeout, sys::MemPool *memPool) : m_pHandler(handler) {

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
            .tv_sec = connectTimeout / 1000 / 1000,
            .tv_usec = (connectTimeout % (1000 * 1000))
    };

    net::NssConfig nc(net::SocketProtocol::Tcp, sspNat, port, net::NetStackWorkerMgrType::Unique,
                      m_pMemPool, std::bind(&KVRpcServerSync::onRecvNetMessage, this, std::placeholders::_1),
                      connTimeout);
    m_pSocketService = net::SocketServiceFactory::CreateService(nc);
    m_pRpcServer = new rpc::RpcServer(workThreadsCnt, m_pSocketService, memPool);
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

rpc::SP_PB_MSG KVRpcServerSync::on_put(rpc::SP_PB_MSG sspMsg) {
    return m_pHandler->OnPut(sspMsg);
}

rpc::SP_PB_MSG KVRpcServerSync::create_put_request() {
    return rpc::SP_PB_MSG(new protocol::PutRequest());
}

rpc::SP_PB_MSG KVRpcServerSync::on_get(rpc::SP_PB_MSG sspMsg) {
    return m_pHandler->OnGet(sspMsg);
}

rpc::SP_PB_MSG KVRpcServerSync::create_get_request() {
    return rpc::SP_PB_MSG(new protocol::GetRequest());
}

rpc::SP_PB_MSG KVRpcServerSync::on_delete(rpc::SP_PB_MSG sspMsg) {
    return m_pHandler->OnDelete(sspMsg);
}

rpc::SP_PB_MSG KVRpcServerSync::create_delete_request() {
    return rpc::SP_PB_MSG(new protocol::DeleteRequest());
}

rpc::SP_PB_MSG KVRpcServerSync::on_scan(rpc::SP_PB_MSG sspMsg) {
    return m_pHandler->OnScan(sspMsg);
}

rpc::SP_PB_MSG KVRpcServerSync::create_scan_request() {
    return rpc::SP_PB_MSG(new protocol::ScanRequest());
}

void KVRpcServerSync::onRecvNetMessage(std::shared_ptr<net::NotifyMessage> sspNM) {
    if (UNLIKELY(m_bStopped)) {
        return;
    }

    m_pRpcServer->HandleMessage(sspNM);
}
} // namespace server
} // namespace minikv