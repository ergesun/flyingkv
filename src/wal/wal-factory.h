/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_WAL_FACTORY_H
#define MINIKV_WAL_FACTORY_H

#include "iwal.h"

namespace minikv {
namespace wal {
class WALFactory {
public:
    static IWal* CreateInstance(const std::string &type, std::string &rootDir, EntryCreateHandler &&handler);
};
}
}

#endif //MINIKV_WAL_FACTORY_H
