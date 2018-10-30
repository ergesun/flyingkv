/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_FACTORY_H
#define FLYINGKV_KV_FACTORY_H

#include "../common/ikv-common.h"

#include "common.h"

namespace flyingkv {
namespace kv {
class KVFactory {
PUBLIC
    common::IKVOperator* CreateInstance(const EngineConstructorParams &param);
};
}
}

#endif //FLYINGKV_KV_FACTORY_H
