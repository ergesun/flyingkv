/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ICHECKPOINT_H
#define FLYINGKV_ICHECKPOINT_H

#include <functional>
#include <vector>

#include "errors.h"
#include "ientries-traveller.h"

namespace flyingkv {
namespace common {
class IEntry;
}

namespace checkpoint {
typedef std::function<void(std::vector<common::IEntry*>)> EntryLoadedCallback;

struct CheckpointResult {
    Code    Rc;
    string  Errmsg;

    explicit CheckpointResult(Code rc) : Rc(rc) {}
    CheckpointResult(Code rc, const string &errmsg) : Rc(rc), Errmsg(errmsg) {}
};

struct LoadCheckpointResult : public CheckpointResult {
    uint64_t MaxId;

    explicit LoadCheckpointResult(uint64_t maxId) : CheckpointResult(Code::OK), MaxId(maxId) {}
    LoadCheckpointResult(Code rc, const string &errmsg) : CheckpointResult(rc, errmsg) {}
};

class ICheckpoint {
PUBLIC
    virtual ~ICheckpoint() = default;

    virtual CheckpointResult Init() = 0;
    virtual LoadCheckpointResult Load(EntryLoadedCallback) = 0;
    virtual CheckpointResult Save(IEntriesTraveller*) = 0;
};
}
}

#endif //FLYINGKV_ICHECKPOINT_H
