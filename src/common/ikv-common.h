/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_SN_INODE_INTERNAL_RPC_HANDLER_H
#define FLYINGKV_SN_INODE_INTERNAL_RPC_HANDLER_H

#include <memory>
#include <google/protobuf/message.h>

#include "../net/notify-message.h"
#include "../rpc/common-def.h"
#include "../codegen/kvrpc.pb.h"

namespace flyingkv {
namespace common {
enum class ReqRespType {
    Put = 0,
    Get,
    Delete,
    Scan
};

typedef std::shared_ptr<google::protobuf::Message> SP_PB_MSG;
typedef std::shared_ptr<protocol::PutRequest> KVPutRequest;
typedef std::shared_ptr<protocol::GetRequest> KVGetRequest;
typedef std::shared_ptr<protocol::DeleteRequest> KVDeleteRequest;
typedef std::shared_ptr<protocol::ScanRequest> KVScanRequest;

/**
 * rpc通信的消息接收接口。
 */
class IKVHandler {
PUBLIC
    virtual ~IKVHandler() = default;
    virtual SP_PB_MSG OnPut(KVPutRequest sspMsg)       = 0;
    virtual SP_PB_MSG OnGet(KVGetRequest sspMsg)       = 0;
    virtual SP_PB_MSG OnDelete(KVDeleteRequest sspMsg) = 0;
    virtual SP_PB_MSG OnScan(KVScanRequest sspMsg)     = 0;
};
} // namespace server
} // namespace flyingkv

#endif //FLYINGKV_SN_INODE_INTERNAL_RPC_HANDLER_H
