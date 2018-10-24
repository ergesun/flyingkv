/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_SN_INODE_INTERNAL_RPC_HANDLER_H
#define MINIKV_SN_INODE_INTERNAL_RPC_HANDLER_H

#include "../net/notify-message.h"
#include "../rpc/common-def.h"

namespace minikv {
namespace kv {
/**
 * rpc通信的消息接收接口。
 */
class IKVHandler {
public:
    virtual ~IKVHandler() = default;
    virtual rpc::SP_PB_MSG OnPut(rpc::SP_PB_MSG sspMsg)                   = 0;
    virtual rpc::SP_PB_MSG OnGet(rpc::SP_PB_MSG sspMsg)                   = 0;
    virtual rpc::SP_PB_MSG OnDelete(rpc::SP_PB_MSG sspMsg)                = 0;
    virtual rpc::SP_PB_MSG OnScan(rpc::SP_PB_MSG sspMsg)                  = 0;
};
} // namespace server
} // namespace minikv

#endif //MINIKV_SN_INODE_INTERNAL_RPC_HANDLER_H
