/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../utils/protobuf-utils.h"
#include "../../common/common-def.h"
#include "../../common/buffer.h"
#include "../../codegen/meta.pb.h"

#include "wal-put-entry.h"
#include "common.h"

namespace flyingkv {
namespace kv {
WalPutEntry::WalPutEntry(sys::MemPool *mp) : m_pMp(mp) {}
WalPutEntry::WalPutEntry(sys::MemPool *mp, std::shared_ptr<protocol::Entry> sspe) : m_pMp(mp), m_sspContent(std::move(sspe)) {}

bool WalPutEntry::Encode(std::shared_ptr<common::Buffer> &sspBuf) {
    if (!m_sspContent) {
        return false;
    }

    int pbSize = m_sspContent->ByteSize();
    auto totalLen = uint32_t(pbSize + 1);
    auto *buffer = new common::Buffer();
    auto mo = m_pMp->Get(uint32_t(pbSize + 1));
    auto start = reinterpret_cast<uchar*>(mo->Pointer());
    buffer->Refresh(start, start + totalLen - 1, start, start + mo->Size() - 1, mo, true);
    *buffer->GetPos() = uchar(EntryType::WalPut);
    buffer->MoveHeadBack(1);
    if (!utils::ProtoBufUtils::Serialize(m_sspContent.get(), buffer)) {
        return false;
    }

    buffer->SetPos(start);
    sspBuf.reset(buffer);
    return true;
}

bool WalPutEntry::Decode(common::Buffer &buffer) {
    if (!buffer.Valid()) {
        return false;
    }

    buffer.MoveHeadBack(1);
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
