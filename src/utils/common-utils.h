/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_UTILS_COMMON_UTILS_H
#define FLYINGKV_UTILS_COMMON_UTILS_H

#include <sys/socket.h>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include "../sys/cctime.h"
#include "../sys/mem-pool.h"

#include "../common/common-def.h"

namespace flyingkv {
namespace common {
    class Buffer;
}

using common::Buffer;
namespace utils {
class CommonUtils {
PUBLIC
    /**
     *
     * @param mpo
     * @param size
     * @return
     */
    static Buffer* GetNewBuffer(sys::MemPoolObject *mpo, uint32_t totalBufferSize);

    /**
     * 判断入参是否为2的N次幂。
     * @param x
     * @return
     */
    static bool IsPowerOfTwo(int x) {
        return (x > 0) && ((x & (x - 1)) == 0);
    }

    // trim from start (in place)
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                        std::not1(std::ptr_fun<int, int>(std::isspace))));
    }

    // trim from end (in place)
    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(),
                             std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    // trim from start (copying)
    static inline std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    static inline std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    static inline std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }

    template <typename T>
    static inline T ToInteger(const std::string &s) {
        std::stringstream ss(s);
        T rs;
        ss >> rs;

        return rs;
    }

    template <typename T>
    static inline std::string ToString(const T &s) {
        std::stringstream ss;
        ss << s;

        return ss.str();
    }
}; // class CommonUtils
}  // namespace utils
}  // namespace flyingkv
#endif //FLYINGKV_UTILS_COMMON_UTILS_H
