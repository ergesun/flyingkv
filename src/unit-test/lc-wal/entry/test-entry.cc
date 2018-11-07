/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../../../common/buffer.h"

#include "test-entry.h"

namespace flyingkv {
namespace waltest {
bool TestEntry::Encode(std::shared_ptr<common::Buffer> &ptr) {
    if (!m_bCanEncode) {
        return false;
    }

    auto *pb = new common::Buffer();
    auto size = m_pContent.size();
    auto buf = new uchar[size];
    memcpy(buf, m_pContent.c_str(), size);
    buf[size] = 0;
    pb->Refresh(buf, buf + size - 1, buf, buf + size - 1, nullptr, true);
    if (0 != m_iIllusorySize) {
        pb->m_pLast = pb->m_pPos + m_iIllusorySize;
    }

    ptr.reset(pb);

    return true;
}

bool TestEntry::Decode(common::Buffer &buffer) {
    if (!m_bCanDecode) {
        return false;
    }

    if (!buffer.Valid()) {
        return true;
    }

    auto str = new char[buffer.AvailableLength() + 1];
    memcpy(str, buffer.m_pPos, size_t(buffer.AvailableLength()));

    m_pContent = str;
    DELETE_ARR_PTR(str);
    return true;
}
}
}
