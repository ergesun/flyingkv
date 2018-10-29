/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_NB_SOCKET_NA_EVENT_MANAGER_H
#define FLYINGKV_NET_CORE_NB_SOCKET_NA_EVENT_MANAGER_H

#include <cstdint>
#include <functional>

#include "../../../../common-def.h"

namespace flyingkv {
namespace sys {
class MemPool;
}

namespace net {
class AFileEventHandler;

typedef std::function<bool(AFileEventHandler*)> ConnectFunc;
typedef std::function<void(AFileEventHandler*)> ConnectHandler, FinishHandler;
class AEventManager {
PUBLIC
    AEventManager(sys::MemPool *memPool, uint32_t maxEvents) :
        m_pMemPool(memPool), m_iMaxEvents(maxEvents) {}
    virtual ~AEventManager() = default;
    virtual bool Start(NonBlockingEventModel m) = 0;
    virtual bool Stop() = 0;
    virtual void AddEvent(AFileEventHandler *socketEventHandler, int cur_mask, int mask) = 0;

PROTECTED
    sys::MemPool      *m_pMemPool;
    uint32_t           m_iMaxEvents;
};
} // namespace net
} // namespace flyingkv

#endif //FLYINGKV_NET_CORE_NB_SOCKET_NA_EVENT_MANAGER_H
