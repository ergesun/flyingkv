/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_SERVER_RF_NODE_H
#define MINIKV_SERVER_RF_NODE_H

#include "../../rpc/rpc-server.h"
#include "../../rpc/common-def.h"

namespace minikv {
namespace kv {
class IKVHandler;
}

namespace server {
class KVRpcServerSync : public IService {
public:
    KVRpcServerSync(kv::IKVHandler *handler, uint16_t workThreadsCnt, uint16_t netIOThreadCnt, uint16_t port,
                    uint32_t connectTimeout/*usec*/, sys::MemPool *memPool = nullptr);
    ~KVRpcServerSync() override;

    bool Start() override;
    bool Stop() override;

private:
    void register_rpc_handlers();
    rpc::SP_PB_MSG on_put(rpc::SP_PB_MSG sspMsg);
    /**
     * for rpc lib to deserialize client request.
     * @return
     */
    rpc::SP_PB_MSG create_put_request();
    rpc::SP_PB_MSG on_get(rpc::SP_PB_MSG sspMsg);
    rpc::SP_PB_MSG create_get_request();
    rpc::SP_PB_MSG on_delete(rpc::SP_PB_MSG sspMsg);
    rpc::SP_PB_MSG create_delete_request();
    rpc::SP_PB_MSG on_scan(rpc::SP_PB_MSG sspMsg);
    rpc::SP_PB_MSG create_scan_request();

    void onRecvNetMessage(std::shared_ptr<net::NotifyMessage> sspNM);

private:
    bool                      m_bStopped        = true;
    uint16_t                  m_iIOThreadsCnt   = 0;
    rpc::RpcServer           *m_pRpcServer      = nullptr;
    kv::IKVHandler           *m_pHandler        = nullptr;/*关联关系，无需释放*/
    net::ISocketService      *m_pSocketService  = nullptr;
    bool                      m_bOwnMemPool     = false;
    sys::MemPool             *m_pMemPool        = nullptr;
};
} // namespace server
} // namespace minikv

#endif //MINIKV_SERVER_RF_NODE_H
