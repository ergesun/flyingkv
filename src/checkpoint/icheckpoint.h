/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ICHECKPOINT_H
#define FLYINGKV_ICHECKPOINT_H

#include <functional>

#include "ientries-traveller.h"

namespace flyingkv {
namespace common {
class IEntry;
}

namespace checkpoint {
typedef std::function<void(common::IEntry*)> EntryLoadedCallback;

class ICheckpoint {
public:
    virtual ~ICheckpoint() = default;

    virtual bool Load(EntryLoadedCallback) = 0;
    virtual bool Save(IEntriesTraveller*) = 0;
};
}
}

#endif //FLYINGKV_ICHECKPOINT_H
