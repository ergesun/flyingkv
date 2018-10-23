//
// Created by sunchao31 on 17-8-9.
//

#ifndef MINIKV_ISERVICE_H
#define MINIKV_ISERVICE_H

namespace minikv {
/**
 * 所有服务的统一接口类。
 */
class IService {
public:
    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual ~IService() {};
};
} // namespace minikv

#endif //MINIKV_ISERVICE_H
