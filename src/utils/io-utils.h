/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_UTILS_IO_UTILS_H
#define FLYINGKV_UTILS_IO_UTILS_H

#include <cstdio>
#include "../sys/mem-pool.h"

#define WriteFileFullyWithFatalLOG(fd, buf, size, filePath)                                         \
        if (-1 == utils::IOUtils::WriteFully((fd), (buf), (size))) {                                \
            auto err = errno;                                                                       \
            LOGFFUN << "write file " << (filePath) << " failed with errmsg " << strerror(err);      \
        }

#define ReadFileFullyWithFatalLOG(fd, buf, size, filePath)                                          \
        if (-1 == utils::IOUtils::ReadFully_V2((fd), (buf), (size))) {                              \
            auto err = errno;                                                                       \
            LOGFFUN << "read file " << (filePath) << " failed with errmsg " << strerror(err);       \
        }

#define LSeekFileWithFatalLOG(fd, offset, where, filePath)                                          \
        if (-1 == lseek((fd), (offset), (where))) {                                                 \
            auto err = errno;                                                                       \
            LOGFFUN << "lseek file " << (filePath) << " failed with errmsg " << strerror(err);      \
        }

#define FSyncFileWithFatalLOG(fd, filePath)                                                         \
        if (-1 == fsync((fd))) {                                                                    \
            auto err = errno;                                                                       \
            LOGFFUN << "fdatasync file " << (filePath) << " failed with errmsg " << strerror(err);  \
        }

#define FDataSyncFileWithFatalLOG(fd, filePath)                                                     \
        if (-1 == fdatasync((fd))) {                                                                \
            auto err = errno;                                                                       \
            LOGFFUN << "fdatasync file " << (filePath) << " failed with errmsg " << strerror(err);  \
        }

namespace flyingkv {
namespace utils {
class IOUtils {
PUBLIC
    /**
     * 将size大小的buf写入fd。
     * @param fd
     * @param buf
     * @param size
     * @return -1失败，否则为size
     */
    static ssize_t WriteFully(int fd, const char *buf, size_t size);

    /**
     * 从fd中读取size大小的buf。
     * ！注意：user负责使用完后释放*buf。
     * @param fd
     * @param buf 输出
     * @param size 想读取的大小
     * @return -1出错，否则为实际读取到的大小。
     */
    static ssize_t ReadFully(int fd, char **buf, size_t size);

    /**
     * 从fd开始处起读取size大小的buf。
     * ！注意：user负责使用完后释放*buf。
     * @param fd
     * @param buf 输出
     * @param size 想读取的大小
     * @return -1出错，否则为实际读取到的大小。
     */
    static ssize_t ReadFully_V2(int fd, char **buf, size_t size);

    /**
     * 从fd开始处起读取size大小的buf。
     * @param fd
     * @param buf 输出
     * @param size 想读取的大小
     * @return -1出错，否则为实际读取到的大小。
     */
    static ssize_t ReadFully_V3(int fd, char *buf, size_t size);

    /**
     * 从fd中读取size大小的buf。
     * @param fd
     * @param buf 输出
     * @param size 想读取的大小
     * @return -1出错，否则为实际读取到的大小。
     */
    static ssize_t ReadFully_V4(int fd, char *buf, size_t size);
};
}
}

#endif //FLYINGKV_UTILS_IO_UTILS_H
