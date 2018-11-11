/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_RAW_PB_ENTRY_ENTRY_H
#define FLYINGKV_KV_RAW_PB_ENTRY_ENTRY_H

#include "../../common/ientry.h"
#include "../../codegen/meta.pb.h"
#include "../../sys/mem-pool.h"

namespace flyingkv {
namespace kv {
class RawPbEntryEntry : public common::IEntry {
PUBLIC
    explicit RawPbEntryEntry(sys::MemPool*);
    RawPbEntryEntry(sys::MemPool*, std::shared_ptr<protocol::Entry>);
    ~RawPbEntryEntry() override = default;

    uint32_t TypeId() override;

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

#endif //FLYINGKV_KV_RAW_PB_ENTRY_ENTRY_H
