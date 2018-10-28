/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_COMMON_KV_ENTRY_H
#define MINIKV_COMMON_KV_ENTRY_H

#include <memory>

namespace minikv {
namespace common {
class Buffer;
class IEntry {
public:
    virtual ~IEntry() = default;

    virtual bool Encode(std::shared_ptr<Buffer>&) = 0;
    virtual bool Decode(const Buffer&) = 0;
};

typedef std::function<common::IEntry*(void)> EntryCreateHandler;
}
}

#endif //MINIKV_COMMON_KV_ENTRY_H