/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_IWAL_H
#define MINIKV_IWAL_H

#include <functional>

#include "ientry.h"

namespace minikv {
namespace wal {
struct WalEntry {
    WalEntry() = delete;
    WalEntry(uint64_t id, IEntry *e) : Id(id), Entry(e) {}
    WalEntry(const WalEntry &we) {
        this->Id = we.Id;
        this->Entry = we.Entry;
    }

    WalEntry(WalEntry &&we) noexcept {
        this->Id = we.Id;
        this->Entry = we.Entry;
        we.Entry = nullptr;
    }

    uint64_t  Id;
    IEntry   *Entry;
};

typedef std::function<IEntry*(void)> EntryCreateHandler;

class IWal {
public:
    virtual ~IWal() = default;

    virtual void AppendEntry(IEntry*) = 0;
    virtual std::vector<WalEntry> Load() = 0;
    virtual void TruncateAhead() = 0;
    virtual void Clean() = 0;
};
}
}

#endif //MINIKV_IWAL_H
