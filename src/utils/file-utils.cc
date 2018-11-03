/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <climits>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <dirent.h>

#include "../common/common-def.h"

#include "file-utils.h"

using std::ifstream;
using std::stringstream;

namespace flyingkv {
namespace utils {
int FileUtils::CreateDir(const string &dir, __mode_t mode) {
    int err;
    struct stat dir_stat;
    // 检查生成文件夹
    if (0 == stat(dir.c_str(), &dir_stat)) { // 如果dir文件(夹)存在
        if (!S_ISDIR(dir_stat.st_mode)) { // 如果不是文件夹
            LOGEFUN << "CreateDir " << dir << " failed because there is a file named it.";
            return -1;
        }
    } else { // 如果dir文件(夹)不存在，则创建文件夹
        if (0 != mkdir(dir.c_str(), mode)) {
            err = errno;
            if (EEXIST == err) {
                return 0;
            } else {
                LOGEFUN << "mkdir " << dir << " failed with errmsg = " << strerror(err);
                return -1;
            }
        }
    }

    return 0;
}

int FileUtils::CreateDirPath(const string &path, __mode_t access_mode) {
    char buffer[PATH_MAX + 1];
    bzero(buffer, sizeof(buffer));
    memcpy(buffer, path.c_str(), PATH_MAX);
    char *p = buffer;
    if ('/' == *p) {
        ++p;
    }

    for (; *p; ++p) {
        if ('/' == *p) {
            *p = 0;
            if (-1 == FileUtils::CreateDir(string(buffer), access_mode)) {
                return -1;
            }
            *p = '/';
        }
    }

    if (-1 == FileUtils::CreateDir(string(buffer), access_mode)) {
        return -1;
    }

    return 0;
}

int FileUtils::RemoveDirectory(const string &path) {
    int rc = CleanDirectory(path);
    if (!rc) {
        rc = rmdir(path.c_str());
        if (0 != rc) {
            rc = errno;
        }
    }

    return rc;
}

int FileUtils::CleanDirectory(const string &path) {
    size_t pathLen = path.size();
    int rc;
    DIR *d = opendir(path.c_str());
    if (d) {
        struct dirent *p;
        rc = 0;
        while (!rc && (p = readdir(d))) {
            int r2 = -1;
            size_t len;
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            len = pathLen + strlen(p->d_name) + 2;
            auto *buf = new char[len]{0};
            struct stat statBuf;
            snprintf(buf, len, "%s/%s", path, p->d_name);
            if (!stat(buf, &statBuf)) {
                if (S_ISDIR(statBuf.st_mode)) {
                    r2 = RemoveDirectory(buf);
                } else {
                    r2 = unlink(buf);
                }
            }

            DELETE_ARR_PTR(buf);
            if (0 != r2) {
                r2 = errno;
            }
            rc = r2;
        }
        closedir(d);
    } else {
        rc = errno;
    }

    return rc;
}

int FileUtils::CollectFileChildren(const string &path, std::function<bool(const string&)> filter,
                               std::vector<string> &rs) {
    size_t pathLen = path.size();
    int rc;
    DIR *d = opendir(path.c_str());
    if (d) {
        struct dirent *p;
        rc = 0;
        while (!rc && (p = readdir(d))) {
            int r2 = -1;
            size_t len;
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            len = pathLen + strlen(p->d_name) + 2;
            auto *buf = new char[len]{0};
            struct stat statBuf;
            snprintf(buf, len, "%s/%s", path, p->d_name);
            if (0 == (r2 = stat(buf, &statBuf)) && S_ISREG(statBuf.st_mode)) {
                std::string tmp(buf);
                if (filter(tmp)) {
                    rs.push_back(std::move(tmp));
                }
            }

            DELETE_ARR_PTR(buf);
            if (0 != r2) {
                r2 = errno;
            }
            rc = r2;
        }
        closedir(d);
    } else {
        rc = errno;
    }

    return rc;
}

ssize_t FileUtils::GetFileSize(const string &path) {
    struct stat s;
    bzero(&s, sizeof(struct stat));
    if (-1 == stat(path.c_str(), &s)) {
        int err = errno;
        LOGEFUN << "stat path " << path << " err with errmsg = " << strerror(err);
        return -1;
    }

    return s.st_size;
}

off_t FileUtils::GetFileSize(int fd) {
    if (0 >= fd) {
        return -1;
    }

    struct stat s;
    bzero(&s, sizeof(struct stat));
    if (-1 == fstat(fd, &s)) {
        int err = errno;
        LOGEFUN << "stat fd " << fd << " err with errmsg = " << strerror(err);
        return -1;
    }

    return s.st_size;
}

string FileUtils::ReadAllString(const string &fp) {
    ifstream fs(fp);

    if (!fs) {
        return "";
    }

    stringstream ss;
    ss << fs.rdbuf();
    return ss.str();
}
} // namespace utils
} // namespace flyingkv
