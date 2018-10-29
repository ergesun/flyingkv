/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_RPC_MESSAGE_HANDLER_H
#define FLYINGKV_RPC_MESSAGE_HANDLER_H

#include <memory>

namespace flyingkv {
namespace net {
    class NotifyMessage;
}

namespace rpc {
class IMessageHandler {
PUBLIC
    virtual ~IMessageHandler() = default;

    virtual void HandleMessage(std::shared_ptr<net::NotifyMessage> sspNM) = 0;
}; // class IMessageHandler
} // namespace rpc
} // namespace flyingkv

#endif //FLYINGKV_RPC_MESSAGE_HANDLER_H
