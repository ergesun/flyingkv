/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_NET_CORE_SOCKET_SERVICE_FACTORY_H
#define MINIKV_NET_CORE_SOCKET_SERVICE_FACTORY_H

#include "isocket-service.h"
#include "notify-message.h"
#include "net-protocol-stacks/nonblocking/nss-config.h"

namespace minikv {
namespace sys {
class MemPool;
}

namespace net {
class INetStackWorkerManager;
class SocketServiceFactory {
public:
    /**
     *
     */
    static ISocketService* CreateService(NssConfig nssConfig);
}; // class SocketServiceFactory
} // namespace net
} // namespace minikv

#endif //MINIKV_NET_CORE_SOCKET_SERVICE_FACTORY_H
