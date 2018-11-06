/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_FACTORY_H
#define FLYINGKV_KV_FACTORY_H

#include "../common/ikv-common.h"

namespace flyingkv {
namespace kv {
class KVConfig;
class KVFactory {
PUBLIC
    common::IKVOperator* CreateInstance(const KVConfig *pc);
};
}
}

#endif //FLYINGKV_KV_FACTORY_H
