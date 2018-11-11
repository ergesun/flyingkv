/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_UTILS_FILE_UTILS_H
#define FLYINGKV_UTILS_FILE_UTILS_H

#include <unistd.h>
#include <sys/stat.h>
#include <functional>
#include <string>
#include <fcntl.h>
#include <unordered_set>
#include <vector>

#include "../sys/spin-lock.h"

using std::string;

namespace flyingkv {
namespace utils {
struct File {
    int fd = -1;
    string path;

    File() = default;
    File(int f, string p) : fd(f), path(std::move(p)) {}
    File(File &f) = default;
    File& operator=(const File &f) = default;
    File(File &&f) noexcept {
        if (this != &f) {
            fd = f.fd;
            path = std::move(f.path);
            f.fd = -1;
        }
    }

    File& operator=(File &&f) {
        if (this != &f) {
            fd = f.fd;
            path = std::move(f.path);
            f.fd = -1;
        }

        return *this;
    }
};

class FileUtils {
PUBLIC
    /**
     * 创建目录
     * @param dir
     * @return 成功返回0,失败返回-1
     */
    static int CreateDir(const string &dir, __mode_t mode);

    /**
     * 递归创建目录路径
     * @param path
     * @param access_mode 访问权限 eg:0755
     * @return 成功返回0,失败返回-1
     */
    static int CreateDirPath(const string &path, __mode_t access_mode);

    /**
     * 删除一个目录
     * @param path
     * @return 成功返回0,否则为errno
     */
    static int RemoveDirectory(const string &path);

    /**
     * 清空一个目录
     * @param path
     * @return 成功返回0,否则为errno
     */
    static int CleanDirectory(const string &path);

    /**
     * 收集目录的子
     * @param path
     * @param ft 类型
     * @param filter 过滤
     * @param rs 结果
     * @return
     */
    static int CollectFileChildren(const string &path, std::function<bool(const string&)> filter,
                               std::vector<std::string> &rs);

    /**
     * 获取文件的大小(bytes)
     * @param path
     * @return 成功返回自然数(>=0)，失败-1。
     */
    static off_t GetFileSize(const string &path);

    /**
     * 获取文件的大小(bytes)
     * @param path
     * @return 成功返回自然数(>=0)，失败-1。
     */
    static ssize_t GetFileSize(int fd);

    /**
     * 读取文件的所有字符。
     * @param fp
     * @return
     */
    static string ReadAllString(const string &fp);

    static inline void GetStat(const string &path, struct stat *st) {
        stat(path.c_str(), st);
    }

    /**
     * 判断是否为文件夹
     * @param path
     * @return 是为true，否则false
     */
    static inline bool IsDir(const string &path) {
        struct stat st;
        return stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode);
    }

    /**
     * 判断是否为文件
     * @param path
     * @return 是为true，否则false
     */
    static inline bool IsFile(const string &path) {
        struct stat st;
        return stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode);
    }

    /**
     * 检查文件是否存在
     * @param path
     * @return 存在为true，否则为false
     */
    static inline bool Exist(const string &path) {
        return access(path.c_str(), F_OK) != -1;
    }

    /**
     * 打开文件。参数参考man 2 open。
     * @param path
     * @param mode read、write
     * @param create 创建与否，与truncate组合与否
     * @param access 文件的访问权限
     * @return 成功返回fd，失败返回-1
     */
    static inline int Open(const string &path, int mode, int create, int access) {
        return open(path.c_str(), mode|create, access);
    }

    static int Unlink(const string &path);

    /**
     * 非阻塞获取一个文件锁。
     * 注意：上锁失败了，如果你要释放这个file，需要使用CloseWithoutRemoveLock方法而不是Close以保证进程内的其他线程的文件锁不被释放。
     * @param fd 目标文件的fd
     * @return 0成功，否则为errno
     */
    static int TryLockFile(const File &file);

    /**
     * TODO(sunchao): 当前并非所有语义都阻塞。如果使用，需要加条件变量使得在本进程范围内的语义也为阻塞。
     * 阻塞获取一个文件锁。
     * 注意：上锁失败了，如果你要释放这个file，需要使用CloseWithoutRemoveLock方法而不是Close以保证进程内的其他线程的文件锁不被释放。
     * @param fd 目标文件的fd
     * @return 0成功，否则为errno
     */
    static int LockFile(const File &file);

    /**
     * 非解除一个文件锁。
     * @param fd 目标文件的fd
     * @return 0成功，否则为errno
     */
    static int UnlockFile(const File &file);

    /**
     * 非阻塞获取一个文件锁。
     * @param path 目标文件的路径
     * @return 返回值的fd为-1失败，否则为文件fd
     */
    static File TryLockPath(const string &path);

    /**
     * TODO(sunchao): 当前并非所有语义都阻塞。如果使用，需要加条件变量使得在本进程范围内的语义也为阻塞。
     * 阻塞获取一个文件锁。
     * 注意：上锁失败了，如果你要释放这个file，需要使用CloseWithoutRemoveLock方法而不是Close以保证进程内的其他线程的文件锁不被释放。
     * @param path 目标文件的路径
     * @return 返回值的fd为-1失败，否则为文件fd
     */
    static File LockPath(const string &path);

    /**
     * 判断文件是否有锁
     * @param file
     * @return -1出错，0没有，1有
     */
    static int IsLocking(const string &file);

    /**
     * TODO(sunchao): 完善这个open以支持创建文件
     * 打开文件。
     * @param path
     * @param oflags
     * @return 失败返回的File对象fd为-1
     */
    static File OpenFile(const char* path, int oflags);

    /**
     * 关闭文件，如果有进程内文件锁，则删除文件锁。
     * @param file
     * @return 成功返回0,失败返回errno
     */
    static int CloseFile(File &file);

    /**
     * 仅关闭文件fd。
     * @param file
     * @return 成功返回0,失败返回errno
     */
    static int CloseWithoutRemoveLock(File &file);

PRIVATE
    static inline bool is_locking_file(const string &path);
    static inline void remove_locking_file_if_exist(const File &file);

PRIVATE
    static std::unordered_set<std::string> s_LockingFiles;
    static sys::spin_lock_t s_sl;
};
}
}

#endif //FLYINGKV_UTILS_FILE_UTILS_H
