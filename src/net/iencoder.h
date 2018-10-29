/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_NET_CORE_ICODEC_H
#define FLYINGKV_NET_CORE_ICODEC_H

#include "../sys/mem-pool.h"

namespace flyingkv {
namespace common {
class Buffer;
}

namespace net {
/**
 * 编码器接口。
 */
class IEncoder {
PUBLIC
    virtual ~IEncoder() {}
    virtual common::Buffer* Encode() = 0;
}; // interface IEncoder
} // namespace net
} // namespace flyingkv

#endif //FLYINGKV_NET_CORE_ICODEC_H
