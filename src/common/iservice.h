//
// Created by sunchao31 on 17-8-9.
//

#ifndef FLYINGKV_ISERVICE_H
#define FLYINGKV_ISERVICE_H

namespace flyingkv {
namespace common {
/**
 * 所有服务的统一接口类。
 */
class IService {
public:
    virtual bool Start() = 0;

    virtual bool Stop() = 0;

    virtual ~IService() {};
};
}
} // namespace flyingkv

#endif //FLYINGKV_ISERVICE_H
