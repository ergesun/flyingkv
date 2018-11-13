/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../entries/raw-pb-entry-entry.h"

#include "mini-kv-traveller.h"

namespace flyingkv {
namespace kv {
MiniKVTraveller::MiniKVTraveller(std::map<MiniKV::Key, RawPbEntryEntry*> *entries, uint64_t maxId,
                                 std::function<void()> prepare, std::function<void()> completePrepare) :
   m_pEntries(entries), m_maxId(maxId), m_prepare(std::move(prepare)), m_completePrepare(std::move(completePrepare)) {
    m_cur = m_pEntries->begin();
}

void MiniKVTraveller::Prepare() {
    m_prepare();
}

void MiniKVTraveller::CompletePrepare() {
    m_completePrepare();
}

common::IEntry* MiniKVTraveller::GetNextEntry() {
    if (m_cur == m_pEntries->end()) {
        return nullptr;
    }

    auto rs =  m_cur->second;
    ++m_cur;
    return rs;
}

bool MiniKVTraveller::Empty() {
    return m_pEntries->empty();
}

uint64_t MiniKVTraveller::MaxId() {
    return m_maxId;
}
}
}
