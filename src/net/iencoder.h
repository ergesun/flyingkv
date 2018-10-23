/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_NET_CORE_ICODEC_H
#define MINIKV_NET_CORE_ICODEC_H

#include "../sys/mem-pool.h"

namespace minikv {
namespace common {
class Buffer;
}

namespace net {
/**
 * 编码器接口。
 */
class IEncoder {
public:
    virtual ~IEncoder() {}
    virtual common::Buffer* Encode() = 0;
}; // interface IEncoder
} // namespace net
} // namespace minikv

#endif //MINIKV_NET_CORE_ICODEC_H
