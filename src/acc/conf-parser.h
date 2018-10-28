/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_ACC_CONF_PARSER_H
#define MINIKV_ACC_CONF_PARSER_H

#include <string>

#include "../cjson/cJSON.h"

namespace minikv {
namespace acc {
class IConfParser {
public:
    virtual ~IConfParser() = default;

    /**
     *
     * @return 如果解析失败，返回false。
     */
    virtual bool Parse(cJSON *blockRoot) = 0;
};
} // namespace acc
} // namespace minikv

#endif //MINIKV_ACC_CONF_PARSER_H
