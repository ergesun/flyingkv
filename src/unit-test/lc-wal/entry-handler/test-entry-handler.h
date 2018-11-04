/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_TEST_ENTRY_HANDLER_H
#define FLYINGKV_TEST_ENTRY_HANDLER_H

#include "../../../common/ientry.h"

namespace flyingkv {
namespace waltest {
class TestEntryHandler {
PUBLIC
    common::IEntry* CreateNewEntry();
};
}
}

#endif //FLYINGKV_TEST_ENTRY_HANDLER_H
