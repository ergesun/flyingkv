/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_CONF_H
#define FLYINGKV_ACC_CONF_H

#include <string>
#include <vector>
#include <cstdint>

#include "../3rd/cjson/cJSON.h"

#include "common-def.h"

namespace flyingkv {
namespace acc {
class cJSONDeleter {
public:
    void operator() (cJSON *root) {
        if (root) {
            cJSON_Delete(root);
        }
    }
};

struct RwtConfig {
    uint32_t Put;
    uint32_t Get;
    uint32_t Delete;
    uint32_t Scan;

    void Parse(const cJSON * const object);
};

struct LimiterConfig {
    LimiterType Type;
};

struct TokenBucketLimiterConfig : public LimiterConfig {
    std::string Name;
    bool        Skip;
    int64_t     MaxSize;
    int64_t     Speed;
    RwtConfig   Rwt;

    void Parse(const cJSON * const object);
};

struct RlcConfig {
    std::string Type;
};

struct SimpleRlcConfig : public RlcConfig {
    bool                            Skip;
    std::vector<LimiterConfig*>     Limiters;

    ~SimpleRlcConfig() {
        for (auto pl : Limiters) {
            delete pl;
        }

        Limiters.clear();
    }

    void Parse(const cJSON * const object);
};

RlcConfig* ParseAccConfig(const std::string &path);
}
}

#endif //FLYINGKV_ACC_CONF_H
