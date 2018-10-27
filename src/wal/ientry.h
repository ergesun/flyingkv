/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_KV_WAL_ENTRY_H
#define MINIKV_KV_WAL_ENTRY_H

#include "../common/buffer.h"

namespace minikv {
namespace wal {
class IEntry {
public:
    virtual ~IEntry() = default;

    virtual bool Encode(std::shared_ptr<common::Buffer>&) = 0;
    virtual bool Decode(const common::Buffer&) = 0;
};
}
}

#endif //MINIKV_KV_WAL_ENTRY_H
