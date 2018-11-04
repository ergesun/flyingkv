/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "test-entry.h"

namespace flyingkv {
namespace waltest {
bool TestEntry::Encode(std::shared_ptr<common::Buffer> &ptr) {
    return false;
}

bool TestEntry::Decode(const common::Buffer &buffer) {
    return false;
}
}
}
