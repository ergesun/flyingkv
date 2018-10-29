/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_POSIX_TCP_CONNECTION_EVENT_HANDLER_H
#define FLYINGKV_NET_CORE_POSIX_TCP_CONNECTION_EVENT_HANDLER_H

#include "../../../../../../../common/common-def.h"
#include "../../abstract-file-event-handler.h"
#include "stack/connection-socket.h"
#include "net-stack-worker.h"

namespace flyingkv {
namespace sys {
class MemPool;
}

namespace net {
class PosixTcpConnectionEventHandler : public AFileEventHandler {
PUBLIC
    PosixTcpConnectionEventHandler(PosixTcpClientSocket *pSocket, sys::MemPool *memPool,
        NotifyMessageCallbackHandler msgCallbackHandler, uint16_t logicPort,
        ConnectFunc onLogicConnect);
    PosixTcpConnectionEventHandler(net_addr_t &peerAddr, int sfd, sys::MemPool *memPool,
    NotifyMessageCallbackHandler msgCallbackHandler, ConnectFunc onLogicConnect);
    ~PosixTcpConnectionEventHandler() override;

    bool Initialize() override;

    bool HandleReadEvent() override;
    bool HandleWriteEvent() override;

    ANetStackMessageWorker *GetStackMsgWorker() override;
    PosixTcpClientSocket* GetSocket() const {
        return m_pClientSocket;
    }

PRIVATE
    PosixTcpClientSocket   *m_pClientSocket = nullptr;
    PosixTcpNetStackWorker *m_pNetStackWorker = nullptr;
    sys::MemPool           *m_pMemPool;
    uint16_t                m_iLogicPort;
};
} // namespace net
} // namespace flyingkv

#endif //FLYINGKV_NET_CORE_POSIX_TCP_CONNECTION_EVENT_HANDLER_H
