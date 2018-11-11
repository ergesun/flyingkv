/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_ENTRIES_DELETE_ENTRY_H
#define FLYINGKV_KV_ENTRIES_DELETE_ENTRY_H

#include "../../common/ientry.h"

namespace flyingkv {
namespace sys {
class MemPool;
}
namespace kv {
class WalDeleteEntry : public common::IEntry {
PUBLIC
    explicit WalDeleteEntry(sys::MemPool*);
    WalDeleteEntry(sys::MemPool*, uchar*, size_t size, bool own);
    ~WalDeleteEntry() override;

    uint32_t TypeId() override;
    bool Encode(std::shared_ptr<common::Buffer>&) override;
    bool Decode(common::Buffer &buffer) override;

    inline uchar* Get() const {
        return m_pContent;
    }

PRIVATE
    sys::MemPool      *m_pMp      = nullptr;
    uchar             *m_pContent = nullptr;
    size_t             m_iSize    = 0;
    bool               m_bOwn     = false;
};
}
}

#endif //FLYINGKV_KV_ENTRIES_DELETE_ENTRY_H
