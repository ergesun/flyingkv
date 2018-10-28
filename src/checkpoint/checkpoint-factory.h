/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_CHECKPOINT_FACTORY_H
#define MINIKV_CHECKPOINT_FACTORY_H

#include <string>

#include "../common/ientry.h"

#include "icheckpoint.h"

namespace minikv {
namespace checkpoint {
class CheckpointFactory {
public:
    static ICheckpoint* CreateInstance(const std::string &type, std::string &rootDir, common::EntryCreateHandler &&handle);
};
}
}

#endif //MINIKV_CHECKPOINT_FACTORY_H
