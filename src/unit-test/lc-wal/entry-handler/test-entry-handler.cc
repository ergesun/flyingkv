/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <memory>

#include "../../../common/buffer.h"

#include "test-entry-handler.h"

namespace flyingkv {
namespace waltest {
common::IEntry *TestEntryHandler::CreateNewEntry() {
    return nullptr;
}
}
}