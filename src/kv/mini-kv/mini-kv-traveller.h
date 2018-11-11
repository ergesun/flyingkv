/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_MINI_KV_TRAVELLER_H
#define FLYINGKV_MINI_KV_TRAVELLER_H

#include "../../checkpoint/ientries-traveller.h"

#include "mini-kv.h"

namespace flyingkv {
namespace kv {
class MiniKVTraveller : public checkpoint::IEntriesTraveller {
PUBLIC
    MiniKVTraveller(const std::map<MiniKV::Key, RawPbEntryEntry*> *entries, uint64_t maxId,
                    std::function<void()> prepare, std::function<void()> completePrepare);
    ~MiniKVTraveller() override = default;

    void Prepare() override;
    void CompletePrepare() override;
    common::IEntry *GetNextEntry() override;
    bool Empty() override;
    uint64_t MaxId() override;

PRIVATE
    const std::map<MiniKV::Key, RawPbEntryEntry*> *m_pEntries;
    std::map<MiniKV::Key, RawPbEntryEntry*>::iterator m_cur;
    uint64_t  m_maxId;
    std::function<void()> m_prepare;
    std::function<void()> m_completePrepare;
};
}
}

#endif //FLYINGKV_MINI_KV_TRAVELLER_H
