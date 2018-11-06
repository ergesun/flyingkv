/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_TEST_ENTRY_H
#define FLYINGKV_TEST_ENTRY_H

#include <string>

#include "../../../common/ientry.h"

namespace flyingkv {
namespace waltest {
class TestEntry : public common::IEntry {
PUBLIC
    TestEntry() = default;
    explicit TestEntry(std::string &&content) : m_pContent(std::move(content)) {}

    bool Encode(std::shared_ptr<common::Buffer> &ptr) override;
    bool Decode(const common::Buffer &buffer) override;

    void SetCanEncode(bool ok) {
        m_bCanEncode = ok;
    }

    void SetCanDecode(bool ok) {
        m_bCanDecode = ok;
    }

    void SetIllusorySize(uint32_t size) {
        m_iIllusorySize = size;
    }

PRIVATE
    std::string m_pContent;
    bool        m_bCanEncode = true;
    bool        m_bCanDecode = true;
    uint32_t    m_iIllusorySize = 0;
};
}
}

#endif //FLYINGKV_TEST_ENTRY_H
