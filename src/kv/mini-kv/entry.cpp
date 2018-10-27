/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../utils/protobuf-utils.h"

#include "entry.h"

namespace minikv {
namespace mnkv {
Entry::Entry(sys::MemPool *mp) : m_pMp(mp) {}
Entry::Entry(sys::MemPool *mp, protocol::Entry *e) : m_pMp(mp), m_pContent(e) {}
Entry::~Entry() {
    DELETE_PTR(m_pContent);
}

bool Entry::Encode(std::shared_ptr<common::Buffer> &sspBuf) {
    if (!m_pContent) {
        return false;
    }

    auto *pb = new common::Buffer();
    if (!utils::ProtoBufUtils::Serialize(m_pContent, pb, m_pMp)) {
        return false;
    }

    sspBuf.reset(pb);
    return true;
}

bool Entry::Decode(const common::Buffer &buffer) {
    if (!buffer.Valid()) {
        return false;
    }

    auto entry = new protocol::Entry();
    if (!utils::ProtoBufUtils::Deserialize(&buffer, entry)) {
        return false;
    }

    m_pContent = entry;
    return true;
}
}
}
