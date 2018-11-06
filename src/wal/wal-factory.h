/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_WAL_FACTORY_H
#define FLYINGKV_WAL_FACTORY_H

#include "../common/ientry.h"

#include "iwal.h"

namespace flyingkv {
namespace wal {
class WalConfig;
class WALFactory {
PUBLIC
    static IWal* CreateInstance(const WalConfig *pc);
};
}
}

#endif //FLYINGKV_WAL_FACTORY_H
