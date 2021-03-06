/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_POSIX_TCPSERVERSOCKET_H
#define FLYINGKV_NET_CORE_POSIX_TCPSERVERSOCKET_H

#include <stdexcept>

#include "connection-socket.h"

namespace flyingkv {
namespace net {
class PosixTcpServerSocket : public PosixTcpConnectionSocket {
PUBLIC
    PosixTcpServerSocket(net_addr_t *localAddr, int maxConns) : PosixTcpConnectionSocket(),
    m_local_addr(*localAddr),
    m_max_listen_conns(maxConns) {}

    /* basic interfaces */
    bool Bind();
    bool Listen();
    int Accept(__SOCKADDR_ARG __addr, socklen_t *__addr_len);
    int Accept4(__SOCKADDR_ARG __addr, socklen_t *__addr_len, int __flags = SOCK_NONBLOCK);
    /* sock opts interfaces */
    bool SetPortReuse();

PRIVATE
    inline bool Connect() {
        throw new std::runtime_error("server socket doesn't support connect api.");
    }

PRIVATE
    net_addr_t  m_local_addr;
    int         m_max_listen_conns;
};
} // namespace net
} // namespace flyingkv

#endif //FLYINGKV_NET_CORE_POSIX_TCPSERVERSOCKET_H
