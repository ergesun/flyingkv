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
class IKVOperator {
PUBLIC
    virtual ~IKVOperator() = default;
    virtual SP_PB_MSG Put(KVPutRequest)       = 0;
    virtual SP_PB_MSG Get(KVGetRequest)       = 0;
    virtual SP_PB_MSG Delete(KVDeleteRequest) = 0;
    virtual SP_PB_MSG Scan(KVScanRequest)     = 0;
};
} // namespace server
} // namespace flyingkv

namespace std {
// 不可以去掉，std::unordered_map会使用它来hash RequestType
template<>
struct hash<flyingkv::common::ReqRespType> {
    uint32_t operator()(const flyingkv::common::ReqRespType &rt) const {
        return (uint32_t) rt;
    }
};
}
#endif //FLYINGKV_SN_INODE_INTERNAL_RPC_HANDLER_H
