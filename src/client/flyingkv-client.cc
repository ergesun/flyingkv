/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <memory>

#include "../net/net-protocol-stacks/nonblocking/nss-config.h"
#include "../net/socket-service-factory.h"
#include "../net/common-def.h"

#include "../sys/mem-pool.h"

#include "rpc/kv-rpc-sync-client.h"
#include "flyingkv-client.h"
#include "../sys/gcc-buildin.h"

namespace flyingkv {
namespace client {
FlyingKVClient::FlyingKVClient(const ClientConfig &cc): m_rpcServerIp(cc.ServerIp), m_rpcServerPort(cc.ServerPort) {
    m_pMemPool = new sys::MemPool();
    m_iIOThreadsCnt = cc.RpcIOThreadsCnt;
    std::shared_ptr<net::net_addr_t> sspNat(nullptr);
    timeval connTimeout = {
            .tv_sec = cc.ConnectTimeoutUs / 1000000,
            .tv_usec = cc.ConnectTimeoutUs % 1000000
    };

    net::NssConfig nc(net::SocketProtocol::Tcp, sspNat, cc.ClientPort, net::NetStackWorkerMgrType::Unique,
                        m_pMemPool, std::bind(&FlyingKVClient::on_recv_net_message, this, std::placeholders::_1),
                        connTimeout);
    m_pSocketService = net::SocketServiceFactory::CreateService(nc);
    sys::cctime procTimeout(cc.ProcessMessageTimeoutMs / 1000, (cc.ProcessMessageTimeoutMs % 1000) * 1000000);
    m_pRpcClient = new KVRpcClientSync(m_pSocketService, procTimeout, m_pMemPool);
}

FlyingKVClient::~FlyingKVClient() {
    DELETE_PTR(m_pRpcClient);
    DELETE_PTR(m_pSocketService);
    DELETE_PTR(m_pMemPool);
}

bool FlyingKVClient::Start() {
    if (!m_bStopped) {
        return true;
    }

    m_bStopped = false;
    hw_sw_memory_barrier();

    if (!m_pRpcClient->Start()) {
        return false;
    }

    return m_pSocketService->Start(m_iIOThreadsCnt, net::NonBlockingEventModel::Posix);
}

bool FlyingKVClient::Stop() {
    if (m_bStopped) {
        return true;
    }

    m_bStopped = true;
    hw_sw_memory_barrier();

    return m_pRpcClient->Stop();
}

std::shared_ptr<protocol::PutResponse> FlyingKVClient::Put(const std::string &k, const std::string &v) throw(rpc::RpcException) {
    auto req = new protocol::PutRequest();
    auto entry = new protocol::Entry();
    entry->set_key(k);
    entry->set_value(v);
    req->set_allocated_entry(entry);
    return m_pRpcClient->Put(std::shared_ptr<protocol::PutRequest>(req), generate_peer());
}

std::shared_ptr<protocol::GetResponse> FlyingKVClient::Get(const std::string &k) throw(rpc::RpcException) {
    auto req = new protocol::GetRequest();
    req->set_key(k);
    return m_pRpcClient->Get(std::shared_ptr<protocol::GetRequest>(req), generate_peer());
}

std::shared_ptr<protocol::DeleteResponse> FlyingKVClient::Delete(const std::string &k) throw(rpc::RpcException) {
    auto req = new protocol::DeleteRequest();
    req->set_key(k);
    return m_pRpcClient->Delete(std::shared_ptr<protocol::DeleteRequest>(req), generate_peer());
}

std::shared_ptr<protocol::ScanResponse> FlyingKVClient::Scan(const std::string &startKey, bool containStartKey,
                                                             protocol::SortOrder order, uint32_t limit) throw(rpc::RpcException) {
    auto req = new protocol::ScanRequest();
    req->set_startkey(startKey);
    req->set_containstartkey(containStartKey);
    req->set_order(order);
    req->set_limit(limit);
    return m_pRpcClient->Scan(std::shared_ptr<protocol::ScanRequest>(req), generate_peer());
}

void FlyingKVClient::on_recv_net_message(std::shared_ptr<net::NotifyMessage> sspNM) {
    if (UNLIKELY(m_bStopped)) {
        return;
    }

    m_pRpcClient->HandleMessage(std::move(sspNM));
}
}
}
