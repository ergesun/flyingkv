/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_MNKV_ENTRY_H
#define FLYINGKV_MNKV_ENTRY_H

#include "../../common/ientry.h"

namespace flyingkv {
namespace sys {
class MemPool;
}
namespace protocol {
class Entry;
}
namespace kv {
class WalPutEntry : public common::IEntry {
PUBLIC
    explicit WalPutEntry(sys::MemPool*);
    WalPutEntry(sys::MemPool*, std::shared_ptr<protocol::Entry>);
    ~WalPutEntry() override = default;

    bool Encode(std::shared_ptr<common::Buffer>&) override;
    bool Decode(common::Buffer &buffer) override;

    inline std::shared_ptr<protocol::Entry> Get() const {
        return m_sspContent;
    }

PRIVATE
    sys::MemPool                      *m_pMp      = nullptr;
    std::shared_ptr<protocol::Entry>   m_sspContent;
};
}
}

#endif //FLYINGKV_MNKV_ENTRY_H
