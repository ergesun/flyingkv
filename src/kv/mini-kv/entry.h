/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_MNKV_ENTRY_H
#define MINIKV_MNKV_ENTRY_H

#include "../../common/ientry.h"
#include "../../codegen/meta.pb.h"

namespace minikv {
namespace mnkv {
class Entry : public common::IEntry {
public:
    explicit Entry(sys::MemPool*);
    Entry(sys::MemPool*, protocol::Entry*);
    ~Entry() override;

    bool Encode(std::shared_ptr<common::Buffer>&) override;
    bool Decode(const common::Buffer &buffer) override;

    inline protocol::Entry* Get() const {
        return m_pContent;
    }

private:
    sys::MemPool      *m_pMp      = nullptr;
    protocol::Entry   *m_pContent = nullptr;
};
}
}

#endif //MINIKV_MNKV_ENTRY_H
