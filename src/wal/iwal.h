/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_IWAL_H
#define MINIKV_IWAL_H

#include <functional>

namespace minikv {
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

class IWal {
public:
    virtual ~IWal() = default;

    /**
     *
     * @return entry id
     */
    virtual uint64_t AppendEntry(common::IEntry*) = 0;
    virtual std::vector<WalEntry> Load() = 0;
    /**
     * truncate log entry in range [-, id]
     * @param id
     * @return
     */
    virtual bool TruncateAhead(uint64_t id) = 0;
    virtual void Reset() = 0;
};
}
}

#endif //MINIKV_IWAL_H
