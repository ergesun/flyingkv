/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_TEST_ENTRY_H
#define FLYINGKV_TEST_ENTRY_H

#include "../../../common/ientry.h"

namespace flyingkv {
namespace waltest {
class TestEntry : public common::IEntry {
PUBLIC
    bool Encode(std::shared_ptr<common::Buffer> &ptr) override;
    bool Decode(const common::Buffer &buffer) override;
};
}
}

#endif //FLYINGKV_TEST_ENTRY_H
