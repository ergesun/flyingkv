/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <memory>

#include "../../../common/buffer.h"

#include "../entry/test-entry.h"

#include "test-entry-handler.h"

namespace flyingkv {
namespace waltest {
common::IEntry *TestEntryHandler::CreateNewEntry(const common::Buffer& b) {
    auto te = new TestEntry();
    te->SetCanDecode(m_bCanDecode);

    return te;
}

common::IEntry* TestEntryHandler::CreateNewEntryWithContent(std::string &&content) {
    auto te = new TestEntry(std::move(content));
    te->SetCanDecode(m_bCanDecode);

    return te;
}

void TestEntryHandler::OnLoad(std::vector<wal::WalEntry> entries) {
    m_vLoadEntries.insert(m_vLoadEntries.end(), entries.begin(), entries.end());
}
}
}
