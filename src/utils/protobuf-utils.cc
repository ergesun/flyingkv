/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../common/buffer.h"

#include "protobuf-utils.h"

namespace minikv {
namespace utils {
bool ProtoBufUtils::Deserialize(const common::Buffer *from, google::protobuf::Message *to) {
    if (UNLIKELY(!to->ParseFromArray(from->GetPos(), from->AvailableLength()))) {
        LOGWFUN << "Missing fields in protocol buffer of " << to->GetTypeName().c_str() << ": " <<
                to->InitializationErrorString().c_str();
        return false;
    }

    return true;
}

bool ProtoBufUtils::Serialize(const google::protobuf::Message *from, common::Buffer *to, sys::MemPool *mp) {
    auto len = static_cast<uint32_t>(from->ByteSize());

    auto mo = mp->Get(len);
    auto start = reinterpret_cast<uchar*>(mo->Pointer());
    if (!from->SerializeToArray(start, len)) {
        return false;
    }
    to->Refresh(start, start + len - 1, start, start + mo->Size() - 1, mo);
    return true;
}

bool ProtoBufUtils::Serialize(const google::protobuf::Message *from, common::Buffer *to) {
    return from->SerializeToArray(to->GetPos(), from->ByteSize());
}
}
}
