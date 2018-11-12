/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_FLYINGKV_CLIENT_H
#define FLYINGKV_FLYINGKV_CLIENT_H

#include <memory>
#include <cstdint>

#include "../net/common-def.h"
#include "../rpc/exceptions.h"
#include "../codegen/kvrpc.pb.h"

#include "config.h"

namespace flyingkv {
namespace net {
class ISocketService;
class NotifyMessage;
}
namespace sys {
class MemPool;
}
namespace client {
class KVRpcClientSync;
class FlyingKVClient {
PUBLIC
    explicit FlyingKVClient(const ClientConfig &cc);
    ~FlyingKVClient();

    bool Start();
    bool Stop();

    std::shared_ptr<protocol::PutResponse> Put(const std::string &k, const std::string &v) throw(rpc::RpcException);
    std::shared_ptr<protocol::GetResponse> Get(const std::string &k) throw(rpc::RpcException);
    std::shared_ptr<protocol::DeleteResponse> Delete(const std::string &k) throw(rpc::RpcException);
    std::shared_ptr<protocol::ScanResponse> Scan(const std::string &startKey, bool containStartKey,
                                                 protocol::SortOrder order, uint32_t limit) throw(rpc::RpcException);

PRIVATE
    void on_recv_net_message(std::shared_ptr<net::NotifyMessage> sspNM);
    inline net::net_peer_info_t generate_peer() {
        return net::net_peer_info_t(m_rpcServerIp, m_rpcServerPort, net::SocketProtocol::Tcp);
    }

PRIVATE
    bool                      m_bStopped        = true;
    uint16_t                  m_iIOThreadsCnt   = 0;
    KVRpcClientSync          *m_pRpcClient      = nullptr;
    net::ISocketService      *m_pSocketService  = nullptr;
    sys::MemPool             *m_pMemPool        = nullptr;
    std::string               m_rpcServerIp;
    uint16_t                  m_rpcServerPort;
};
}
}

#endif //FLYINGKV_FLYINGKV_CLIENT_H
