/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_ICHECKPOINT_H
#define MINIKV_ICHECKPOINT_H

#include <functional>

#include "ientries-traveller.h"

namespace minikv {
namespace common {
class IEntry;
}

namespace checkpoint {
typedef std::function<void(common::IEntry*)> EntryLoadedCallback;

class ICheckpoint {
public:
    virtual ~ICheckpoint() = default;

    virtual void Load(EntryLoadedCallback) = 0;
    virtual void Save(IEntriesTraveller*) = 0;
};
}
}

#endif //MINIKV_ICHECKPOINT_H
