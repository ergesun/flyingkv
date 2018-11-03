/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include "../utils/file-utils.h"
#include "../common/common-def.h"

#include "config.h"

namespace flyingkv {
namespace acc {
RlcConfig* ParseAccConfig(const std::string &path) {
    auto confJsonString = utils::FileUtils::ReadAllString(path);
    if (path.empty()) {
        LOGFFUN << "read acc conf file " << path.c_str() << " failed!";
    }

    LOGIFUN << "acc json conf string = " << confJsonString;
    auto root = cJSON_Parse(confJsonString.c_str());
    std::unique_ptr<cJSON, cJSONDeleter> sup_root(root);
    auto accNode = cJSON_GetObjectItem(root, "acc");
    auto typeItem = cJSON_GetObjectItem(accNode, "type");
    RlcConfig *pRc = nullptr;
    if (std::string(typeItem->valuestring) == "simple") {
        auto src = new SimpleRlcConfig();
        src->Type = "simple";
        src->Parse(accNode);
        pRc = src;
    } else {
        LOGFFUN << "not support acc type.";
    }

    return pRc;
}

void SimpleRlcConfig::Parse(const cJSON *const accNode) {
    auto skipItem = cJSON_GetObjectItem(accNode, "skip");
    Skip = (1 == skipItem->valueint);
    auto limitersItem = cJSON_GetObjectItem(accNode, "limiters");
    auto size = cJSON_GetArraySize(limitersItem);
    for (int i = 0; i < size; ++i) {
        auto arrItem = cJSON_GetArrayItem(limitersItem, i);
        auto typeItem = cJSON_GetObjectItem(arrItem, "type");
        auto limiterType = typeItem->valuestring;
        if (std::string(limiterType) == "token-bucket") {
            auto tbc = new TokenBucketLimiterConfig();
            tbc->Parse(arrItem);
            Limiters.push_back(tbc);
        } else {
            LOGFFUN << "not support limiter type " << limiterType;
        }
    }
}

void TokenBucketLimiterConfig::Parse(const cJSON *const object) {
    Type = LimiterType::TOKEN_BUCKET;
    auto skipItem = cJSON_GetObjectItem(object, "skip");
    Skip = (1 == skipItem->valueint);
    auto nameItem = cJSON_GetObjectItem(object, "name");
    Name = nameItem->valuestring;
    auto msItem = cJSON_GetObjectItem(object, "ms"); // max tokens size
    MaxSize = msItem->valueint;
    auto speedItem = cJSON_GetObjectItem(object, "speed");
    Speed = speedItem->valueint;
    auto rwtItem = cJSON_GetObjectItem(object, "rwt");
    Rwt.Parse(rwtItem);
}

void RwtConfig::Parse(const cJSON *const object) {
    auto putItem = cJSON_GetObjectItem(object, "put");
    Put = uint32_t(putItem->valueint);
    auto getItem = cJSON_GetObjectItem(object, "get");
    Put = uint32_t(getItem->valueint);
    auto deleteItem = cJSON_GetObjectItem(object, "delete");
    Put = uint32_t(deleteItem->valueint);
    auto scanItem = cJSON_GetObjectItem(object, "scan");
    Put = uint32_t(scanItem->valueint);
}
}
}
