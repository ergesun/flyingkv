/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_PROTOBUF_UTILS_H
#define MINIKV_PROTOBUF_UTILS_H

#include <google/protobuf/message.h>

namespace minikv {
namespace sys {
class MemPool;
}
namespace utils {
class Buffer;
class ProtoBufUtils {
public:
    static bool Deserialize(const common::Buffer *from, google::protobuf::Message *to);
    static void Serialize(const google::protobuf::Message *from, common::Buffer *to, sys::MemPool *mp);
    static void Serialize(const google::protobuf::Message *from, common::Buffer *to);
};
}
}

#endif //MINIKV_PROTOBUF_UTILS_H
