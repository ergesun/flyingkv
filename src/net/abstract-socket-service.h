/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_ABSTRACT_SOCKET_SERVICE_H
#define FLYINGKV_NET_CORE_ABSTRACT_SOCKET_SERVICE_H

#include "../common/common-def.h"
#include "isocket-service.h"

namespace flyingkv {
namespace net {
class ASocketService : public ISocketService {
PUBLIC
    ASocketService(SocketProtocol sp, std::shared_ptr<net_addr_t> sspNat) : m_sp(sp), m_nlt(sspNat) {}

PROTECTED
    /**
     * service的socket类型。
     */
    SocketProtocol              m_sp;
    /**
     * 本地监听的地址协议信息。
     */
    std::shared_ptr<net_addr_t> m_nlt;
};
} // namespace net
} // namespace flyingkv

#endif //FLYINGKV_NET_CORE_ABSTRACT_SOCKET_SERVICE_H
