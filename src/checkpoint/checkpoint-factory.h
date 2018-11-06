/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_CHECKPOINT_FACTORY_H
#define FLYINGKV_CHECKPOINT_FACTORY_H

#include <string>

#include "../common/ientry.h"

#include "icheckpoint.h"

namespace flyingkv {
namespace checkpoint {
class CheckpointConfig;
class CheckpointFactory {
PUBLIC
    static ICheckpoint* CreateInstance(const CheckpointConfig *pc);
};
}
}

#endif //FLYINGKV_CHECKPOINT_FACTORY_H
