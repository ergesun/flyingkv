/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_NPS_NB_NA_SIMPLE_EVENT_HANDLER_H
#define FLYINGKV_NET_CORE_NPS_NB_NA_SIMPLE_EVENT_HANDLER_H

#include "../../abstract-file-event-handler.h"

namespace flyingkv {
namespace net {
class PosixLocalReadEventHandler : public AFileEventHandler {
public:
    PosixLocalReadEventHandler(int fd);
    ~PosixLocalReadEventHandler();

    bool Initialize() override;

    bool HandleReadEvent() override;
    bool HandleWriteEvent() override;

    ANetStackMessageWorker *GetStackMsgWorker() override;

private:
    int m_fd;
};
} // namespace net
} // namespace flyingkv

#endif //FLYINGKV_NET_CORE_NPS_NB_NA_SIMPLE_EVENT_HANDLER_H
