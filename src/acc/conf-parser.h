/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_ACC_CONF_PARSER_H
#define FLYINGKV_ACC_CONF_PARSER_H

#include <string>

#include "../3rd/cjson/cJSON.h"

namespace flyingkv {
namespace acc {
class IConfParser {
PUBLIC
    virtual ~IConfParser() = default;

    /**
     *
     * @return 如果解析失败，返回false。
     */
    virtual bool Parse(cJSON *blockRoot) = 0;
};
} // namespace acc
} // namespace flyingkv

#endif //FLYINGKV_ACC_CONF_PARSER_H
