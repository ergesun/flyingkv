/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_KV_COMMON_H
#define FLYINGKV_KV_COMMON_H

namespace flyingkv {
namespace kv {
enum class EntryType {
    RawEntry  = 1,
    WalPut    = 2,
    WalDelete = 3
};
}
}

#endif //FLYINGKV_KV_COMMON_H
