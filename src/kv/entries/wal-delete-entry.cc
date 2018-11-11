/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../common/buffer.h"

#include "wal-delete-entry.h"
#include "common.h"


namespace flyingkv {
namespace kv {
WalDeleteEntry::WalDeleteEntry(sys::MemPool *mp) : m_pMp(mp) {}
WalDeleteEntry::WalDeleteEntry(sys::MemPool *mp, uchar *content, size_t size, bool own) :
        m_pMp(mp), m_pContent(content), m_iSize(size), m_bOwn(own) {}
WalDeleteEntry::~WalDeleteEntry() {
    if (m_bOwn) {
        DELETE_PTR(m_pContent);
    }
}

uint32_t WalDeleteEntry::TypeId() {
    return uint32_t(EntryType::WalDelete);
}

bool WalDeleteEntry::Encode(std::shared_ptr<common::Buffer> &sspBuf) {
    if (!m_pContent) {
        return false;
    }

    auto totalLen = uint32_t(m_iSize + 1);
    auto *buffer = new common::Buffer();
    auto mo = m_pMp->Get(uint32_t(m_iSize + 1));
    auto start = reinterpret_cast<uchar*>(mo->Pointer());
    buffer->Refresh(start, start + totalLen - 1, start, start + mo->Size() - 1, mo, true);
    *buffer->GetPos() = uchar(EntryType::WalDelete);
    buffer->MoveHeadBack(1);
    memcpy(buffer->GetPos(), m_pContent, m_iSize);
    buffer->SetPos(start);

    sspBuf.reset(buffer);
    return true;
}

bool WalDeleteEntry::Decode(common::Buffer &buffer) {
    if (!buffer.Valid()) {
        return false;
    }

    buffer.MoveHeadBack(1);
    if (!buffer.Valid()) {
        return false;
    }

    auto len = buffer.AvailableLength();
    m_pContent = new uchar[len + 1];
    m_pContent[len] = '\0';
    m_bOwn = true;
    memcpy(m_pContent, buffer.GetPos(), size_t(len));

    return true;
}
}
}