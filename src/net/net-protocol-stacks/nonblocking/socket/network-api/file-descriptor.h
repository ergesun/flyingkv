/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_SOCKETAPI_SOCKET_H
#define FLYINGKV_NET_CORE_SOCKETAPI_SOCKET_H

#include "../../../../common-def.h"

namespace flyingkv {
namespace net {
class FileDescriptor {
PUBLIC
    FileDescriptor() = default;
    FileDescriptor(int sd, net_peer_info_t realPeer) : m_fd(sd), m_real_peer(realPeer) {}

    /**
     * 获取内部的文件描述符。
     * @return
     */
    inline int GetFd() {
        return m_fd;
    }

    /**
     * 获取实际文件描述符对应的端点信息。
     * @return
     */
    inline net_peer_info_t GetRealPeerInfo() {
        return m_real_peer;
    }

    /**
     *
     * @param peer
     */
    inline void SetLogicPeerInfo(net_peer_info_t &&peer) {
        m_logic_peer = std::move(peer);
    }

    /**
     * 获取逻辑文件描述符对应的端点信息。
     * @return
     */
    inline net_peer_info_t GetLogicPeerInfo() {
        return m_logic_peer;
    }

PROTECTED
    int               m_fd = -1;
    net_peer_info_t   m_real_peer;
    net_peer_info_t   m_logic_peer;
};
}
}

#endif //FLYINGKV_NET_CORE_SOCKETAPI_SOCKET_H
