/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_IWAL_H
#define FLYINGKV_IWAL_H

#include <functional>
#include <vector>

#include "errors.h"

namespace flyingkv {
namespace common {
class IEntry;
}
namespace wal {
struct WalResult {
    Code    Rc;
    string  Errmsg;

    WalResult() = default;
    explicit WalResult(Code rc) : Rc(rc) {}
    WalResult(Code rc, const string &errmsg) : Rc(rc), Errmsg(errmsg) {}

    WalResult(const WalResult &b) {
        this->Rc = b.Rc;
        this->Errmsg = b.Errmsg;
    }
};

struct AppendEntryResult : public WalResult {
    uint64_t   EntryId;

    AppendEntryResult() = default;
    explicit AppendEntryResult(uint64_t entryId) : WalResult(Code::OK), EntryId(entryId) {}
    AppendEntryResult(Code rc, const string &errmsg) : WalResult(rc, errmsg) {}

    AppendEntryResult(const AppendEntryResult& b) {
        WalResult::Rc = b.Rc;
        WalResult::Errmsg = b.Errmsg;
        this->EntryId = b.EntryId;
    };
    AppendEntryResult& operator=(AppendEntryResult &&aer) {
        this->Rc = aer.Rc;
        this->Errmsg = std::move(aer.Errmsg);
        this->EntryId = aer.EntryId;

        return *this;
    }
};

struct SizeResult : public WalResult {
    uint64_t   Size;

    SizeResult() = default;
    explicit SizeResult(uint64_t size) : WalResult(Code::OK), Size(size) {}
    SizeResult(Code rc, const string &errmsg) : WalResult(rc, errmsg) {}

    SizeResult(SizeResult &&sr) : WalResult(sr.Rc, std::move(sr.Errmsg)), Size(sr.Size) {}
};

typedef WalResult LoadResult;
typedef WalResult TruncateResult;

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

    WalEntry& operator=(const WalEntry &a) noexcept {
        this->Id = a.Id;
        this->Entry = a.Entry;

        return *this;
    }

    uint64_t          Id;
    common::IEntry   *Entry;
};

typedef std::function<void(std::vector<WalEntry>)> WalEntryLoadedCallback;

class IWal {
PUBLIC
    virtual ~IWal() = default;

    virtual WalResult Init() = 0;
    virtual LoadResult Load(const WalEntryLoadedCallback&) = 0;
    /**
     *
     * @return entry id
     */
    virtual AppendEntryResult AppendEntry(common::IEntry*) = 0;
    /**
     * truncate log entry in range [-, id]
     * @param id
     * @return
     */
    virtual TruncateResult Truncate(uint64_t id) = 0;
    virtual SizeResult Size() = 0;
};
}
}

#endif //FLYINGKV_IWAL_H
