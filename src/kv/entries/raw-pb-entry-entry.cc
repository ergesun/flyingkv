/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../common/buffer.h"
#include "../../utils/protobuf-utils.h"
#include "common.h"

#include "raw-pb-entry-entry.h"

namespace flyingkv {
namespace kv {
RawPbEntryEntry::RawPbEntryEntry(sys::MemPool *mp) : m_pMp(mp) {}
RawPbEntryEntry::RawPbEntryEntry(sys::MemPool *mp, std::shared_ptr<protocol::Entry> sspe) : m_pMp(mp), m_sspContent(std::move(sspe)) {}

uint32_t RawPbEntryEntry::TypeId() {
    return uint32_t(EntryType::RawEntry);
}

bool RawPbEntryEntry::Encode(std::shared_ptr<common::Buffer> &sspBuf) {
    if (!m_sspContent) {
        return false;
    }

    auto *buffer = new common::Buffer();
    if (!utils::ProtoBufUtils::Serialize(m_sspContent.get(), buffer, m_pMp)) {
        return false;
    }

    sspBuf.reset(buffer);
    return true;
}

bool RawPbEntryEntry::Decode(common::Buffer &buffer) {
    if (!buffer.Valid()) {
        return false;
    }

    auto entry = new protocol::Entry();
    if (!utils::ProtoBufUtils::Deserialize(&buffer, entry)) {
        return false;
    }

    m_sspContent.reset(entry);
    return true;
}
}
}