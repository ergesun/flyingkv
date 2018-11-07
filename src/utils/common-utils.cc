/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <iostream>

#include "../common/buffer.h"
#include "common-utils.h"

namespace flyingkv {
namespace utils {
Buffer* CommonUtils::GetNewBuffer(sys::MemPoolObject *mpo, uint32_t totalBufferSize) {
    auto bufferStart = (uchar*)(mpo->Pointer());
    auto bufferEnd = bufferStart + totalBufferSize - 1;
    return new Buffer(nullptr, nullptr, bufferStart, bufferEnd, mpo, true);
}
} // namespace utils
} // namespace flyingkv
