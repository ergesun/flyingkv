/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_PROTOBUF_UTILS_H
#define FLYINGKV_PROTOBUF_UTILS_H

#include <google/protobuf/message.h>

namespace flyingkv {
namespace common {
class Buffer;
}
namespace sys {
class MemPool;
}
namespace utils {
class Buffer;
class ProtoBufUtils {
public:
    static bool Deserialize(const common::Buffer *from, google::protobuf::Message *to);
    static bool Serialize(const google::protobuf::Message *from, common::Buffer *to, sys::MemPool *mp);
    static bool Serialize(const google::protobuf::Message *from, common::Buffer *to);
};
}
}

#endif //FLYINGKV_PROTOBUF_UTILS_H
