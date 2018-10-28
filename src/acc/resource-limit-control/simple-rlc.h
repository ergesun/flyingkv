/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_ACC_SIMPLE_RLC_H
#define MINIKV_ACC_SIMPLE_RLC_H

#include "rlc.h"

namespace minikv {
namespace acc {
class SimpleRlc : public IRlc {
public:
    SimpleRlc();
    ~SimpleRlc() override;


};
}
}

#endif //MINIKV_ACC_SIMPLE_RLC_H
