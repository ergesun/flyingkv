/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_RPC_HANDLER_H
#define FLYINGKV_RPC_HANDLER_H

#include <memory>
#include <cassert>

#include "common-def.h"
#include "../common/ikv-common.h"

namespace google {
namespace protobuf {
class Message;
}
}

namespace flyingkv {
namespace common {
class Buffer;
}

namespace net {
class SndMessage;
}

namespace rpc {
class IRpcHandler {
public:
    virtual ~IRpcHandler() = default;
    /**
     * rpc事件的回调。
     * @param b 对端的消息内容buffer。
     * @param sm 本端的响应消息体。
     */
    virtual common::SP_PB_MSG Handle(common::SP_PB_MSG req) = 0;
    virtual common::SP_PB_MSG CreateMessage() = 0;
};

/**
 * 辅助的通用rpc handler。
 */
class TypicalRpcHandler : public IRpcHandler {
public:
    typedef std::function<common::SP_PB_MSG(common::SP_PB_MSG)> RpcHandle;
    typedef std::function<common::SP_PB_MSG(void)> RequestCreator;

public:
    TypicalRpcHandler(RpcHandle handle, RequestCreator requestCreator) :
        m_handle(std::move(handle)), m_requestCreator(std::move(requestCreator)) {
        assert((nullptr != m_handle) && (nullptr != m_requestCreator));
    }

    common::SP_PB_MSG Handle(common::SP_PB_MSG req) override;
    common::SP_PB_MSG CreateMessage() override;

private:
    RpcHandle             m_handle;
    RequestCreator        m_requestCreator;
};
}
}

#endif //FLYINGKV_RPC_HANDLER_H
