/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_CHECKPOINT_ENTRIES_TRAVELLER_H
#define FLYINGKV_CHECKPOINT_ENTRIES_TRAVELLER_H

namespace flyingkv {
namespace common {
class IEntry;
}
namespace checkpoint {
class IEntriesTraveller {
public:
    virtual ~IEntriesTraveller() = default;

    virtual common::IEntry* GetNextEntry() = 0;
    virtual bool Empty() = 0;
};
}
}

#endif //FLYINGKV_CHECKPOINT_ENTRIES_TRAVELLER_H
