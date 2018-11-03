/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_IWAL_H
#define FLYINGKV_IWAL_H

#include <functional>
#include <vector>

namespace flyingkv {
namespace common {
class IEntry;
}
namespace wal {
struct WalEntry {
    WalEntry() = delete;
    WalEntry(uint64_t id, common::IEntry *e) : Id(id), Entry(e) {}
    WalEntry(const WalEntry &we) {
        this->Id = we.Id;
        this->Entry = we.Entry;
    }

    WalEntry(WalEntry &&we) noexcept {
        this->Id = we.Id;
        this->Entry = we.Entry;
        we.Entry = nullptr;
    }

    uint64_t          Id;
    common::IEntry   *Entry;
};

typedef std::function<void(std::vector<WalEntry>)> WalEntryLoadedCallback;

class IWal {
PUBLIC
    virtual ~IWal() = default;

    /**
     *
     * @return entry id
     */
    virtual uint64_t AppendEntry(common::IEntry*) = 0;
    virtual void Load(const WalEntryLoadedCallback&) = 0;
    /**
     * truncate log entry in range [-, id]
     * @param id
     * @return
     */
    virtual bool TruncateAhead(uint64_t id) = 0;
};
}
}

#endif //FLYINGKV_IWAL_H
