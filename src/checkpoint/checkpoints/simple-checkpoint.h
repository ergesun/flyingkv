/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_SIMPLE_CHECKPOINT_H
#define MINIKV_SIMPLE_CHECKPOINT_H

#include "../icheckpoint.h"

namespace minikv {
namespace checkpoint {
class SimpleCheckpoint : public ICheckpoint {
public:
    SimpleCheckpoint();
    ~SimpleCheckpoint() override;

    void Load(EntryLoadedCallback callback) override;
    void Save(IEntriesTraveller *traveller) override;

private:
    std::string                           m_sRootDir;
    std::string                           m_sLogFilePath;
};
}
}

#endif //MINIKV_SIMPLE_CHECKPOINT_H
