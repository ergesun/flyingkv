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
common::IEntry *TestEntryHandler::CreateNewEntry() {
    return new TestEntry();
}

common::IEntry* TestEntryHandler::CreateNewEntryWithContent(std::string &&content) {
    return new TestEntry(std::move(content));
}

void TestEntryHandler::OnLoad(std::vector<wal::WalEntry> entries) {
    m_vLoadEntries.insert(m_vLoadEntries.end(), entries.begin(), entries.end());
}
}
}
