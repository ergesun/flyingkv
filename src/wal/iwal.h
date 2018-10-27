/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_IWAL_H
#define MINIKV_IWAL_H



namespace minikv {
namespace wal {
class IWal {
public:
    virtual ~IWal() = default;

    virtual void PutEntry() = 0;
};
}
}

#endif //MINIKV_IWAL_H
