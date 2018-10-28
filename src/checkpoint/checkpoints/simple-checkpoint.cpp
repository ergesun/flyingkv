/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <sys/mman.h>
#include "../../common/common-def.h"
#include "../../utils/file-utils.h"
#include "../../utils/io-utils.h"
#include "../../utils/codec-utils.h"

#include "../ientries-traveller.h"

#include "simple-checkpoint.h"
#include "../../common/buffer.h"
#include "../../common/global-vars.h"

namespace minikv {
namespace checkpoint {
SimpleCheckpoint::SimpleCheckpoint(const std::string &rootDir, common::EntryCreateHandler &&ech) :
        m_sRootDir(rootDir), m_entryCreator(std::move(ech)) {
    if (rootDir.empty()) {
        LOGFFUN << "simple checkpoint root dir is empty";
    }

    m_sRootDir = rootDir;
    if (-1 == utils::FileUtils::CreateDirPath(m_sRootDir.c_str(), 0775)) {
        LOGFFUN << "create simple checkpoint dir " << m_sRootDir.c_str() << " failed.";
    }

    m_sCpFilePath = m_sRootDir + "/" + SMCP_PREFIX_NAME;
    m_sNewLogFilePath = m_sCpFilePath + SMCP_NEW_FILE_SUFFIX;
    m_sStatusFilePath = m_sRootDir + "/" +SMCP_COMPLETE_FLAG;
}

SimpleCheckpoint::~SimpleCheckpoint() {

}

// TODO(sunchao): 把load定义一个模式抽象一下？免得和log的load逻辑重复
bool SimpleCheckpoint::Load(EntryLoadedCallback callback) {
    if (utils::FileUtils::Exist(m_sCpFilePath)) {
        int fd = utils::FileUtils::Open(m_sCpFilePath, O_RDONLY, 0, 0);
        if (-1 == fd) {
            auto err = errno;
            LOGFFUN << "open sm checkpoint file " << m_sCpFilePath.c_str() << " failed with errmsg " << strerror(err);
        }

        // 1. check if file is empty.
        auto fileSize = utils::FileUtils::GetFileSize(fd);
        if (-1 == fileSize) {
            auto err = errno;
            LOGFFUN << "get file size for " << m_sCpFilePath.c_str() << " failed with errmsg " << strerror(err);
        }
        if (0 == fileSize) {
            LOGDFUN3("sm checkpoint file ", m_sCpFilePath.c_str(), " is empty!");
            return true;
        }

        // 2. check if file header is corrupt.
        if (fileSize < SMCP_MAGIC_NO_LEN) { // 失败了人工处理
            LOGFFUN << "sm checkpoint file " << m_sCpFilePath.c_str() << " header is corrupt!";
        }

        // TODO(sunchao): 如果以后扩展这个checkpoint，支持大的checkpoint，就不要mmap了
        auto mapRet = mmap(nullptr, (size_t)fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
        if (MAP_FAILED == mapRet) {
            auto err = errno;
            LOGFFUN << "mmap sm checkpoint " << m_sCpFilePath.c_str() << " failed with errmsg " << strerror(err);
        }

        auto buf = (uchar*)mapRet;
        uint32_t offset = 0;
        uint32_t magicNo = ByteOrderUtils::ReadUInt32(buf);
        if (SMCP_MAGIC_NO != magicNo) {
            LOGFFUN << "sm checkpoint " << m_sCpFilePath.c_str() << " header is corrupt!";
        }

        offset += SMCP_MAGIC_NO_LEN;
        common::Buffer b;
        // 3. load sm checkpoint content
        while (offset <= fileSize - 1) {
            // 3.1 read sm checkpoint entry len
            auto len = ByteOrderUtils::ReadUInt32(buf + offset);
            auto startPos = ByteOrderUtils::ReadUInt32(buf + offset + len);
            if (startPos != offset) {
                LOGFFUN << "parse sm checkpoint " << m_sCpFilePath.c_str() << " failed at offset " << offset;
            }

            auto startPtr = buf + offset;
            auto endPtr = buf + len - 1;
            b.Refresh(startPtr, endPtr, startPtr, endPtr, nullptr);
            auto entry = m_entryCreator();
            if (!entry->Decode(b)) {
                LOGFFUN << "deserialize sm checkpoint entry at offset " << offset << " failed!";
            }

            offset += (len + sizeof(uint32_t));
            callback(entry);
        }

        if (-1 == munmap(mapRet, size_t(fileSize))) {
            auto err = errno;
            LOGFFUN << "munmap file " << m_sCpFilePath.c_str() << " failed with errmsg " << strerror(err);
        }

        if (-1 != fd) {
            close(fd);
        }
    }

    return true;
}

// TODO(sunchao): 有时间优化为batch写
bool SimpleCheckpoint::Save(IEntriesTraveller *traveller) {
    if (traveller->Empty()) {
        return true;
    }

    int fd = this->create_new_checkpoint();
    if (-1 == fd) {
        return false;
    }

    if (!this->rm_completed_status()) {
        return false;
    }

    common::FileCloser fc(fd);
    common::IEntry *entry = nullptr;
    uint32_t offset = SMCP_MAGIC_NO_LEN;
    LSeekFileWithFatalLOG(fd, SMCP_MAGIC_NO_LEN, SEEK_SET, m_sNewLogFilePath);
    while (entry = traveller->GetNextEntry()) {
        std::shared_ptr<common::Buffer> eb;
        if (UNLIKELY(!entry->Encode(eb))) {
            LOGFFUN << "simple wal encode entry error";
        }

        auto rawEntrySize = eb->AvailableLength();
        auto walEntrySize = walEntrySize + 8/*len 4 + start pos 4*/;
        // mpo will be Putted in Buffer 'b'.
        auto mpo = common::g_pMemPool->Get(walEntrySize);
        auto bufferStart = (uchar*)(mpo->Pointer());
        common::Buffer wb;
        wb.Refresh(bufferStart, bufferStart + walEntrySize - 1, bufferStart, bufferStart + walEntrySize - 1, mpo);
        // size
        ByteOrderUtils::WriteUInt32(wb.GetPos(), uint32_t(rawEntrySize));
        wb.SetPos(wb.GetPos() + 4);

        // content
        memcpy(eb->GetPos(), wb.GetPos(), size_t(rawEntrySize));
        wb.SetPos(wb.GetPos() + rawEntrySize);

        // entry offset
        ByteOrderUtils::WriteUInt32(wb.GetPos(), offset);

        WriteFileFullyWithFatalLOG(fd, (char*)(wb.GetStart()), walEntrySize, m_sCpFilePath.c_str());
        FDataSyncFileWithFatalLOG(fd, m_sCpFilePath.c_str());\
        offset += walEntrySize;
    }

    if (-1 == unlink(m_sCpFilePath.c_str())) {
        LOGEFUN << "unlink old checkpoint " << m_sCpFilePath.c_str() << " error " << strerror(errno);
        return false;
    }

    if (-1 == rename(m_sNewLogFilePath.c_str(), m_sCpFilePath.c_str())) {
        LOGEFUN << "rename new checkpoint " << m_sNewLogFilePath.c_str() << " error " << strerror(errno);
        return false;
    }

    if (!create_completed_status()) {
        LOGEFUN << "create checkpoint completed status file error " << strerror(errno);
        return false;
    }
    return true;
}

int SimpleCheckpoint::create_new_checkpoint() {
    int fd = utils::FileUtils::Open(m_sNewLogFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644)
    LOGDFUN2("create sm checkpoint file ", m_sNewLogFilePath.c_str());
    if (-1 == fd) {
        auto err = errno;
        LOGEFUN << "create sm checkpoint file " << m_sNewLogFilePath.c_str() << " failed with errmsg " << strerror(err);
        return -1;
    }

    // write file header.
    uchar header[SMCP_MAGIC_NO_LEN];
    ByteOrderUtils::WriteUInt32(header, SMCP_MAGIC_NO);
    WriteFileFullyWithFatalLOG(fd, (char*)header, SMCP_MAGIC_NO_LEN, m_sNewLogFilePath.c_str());

    return fd;
}

bool SimpleCheckpoint::is_completed() {
    return utils::FileUtils::Exist(m_sStatusFilePath);
}

bool SimpleCheckpoint::rm_completed_status() {
    if (utils::FileUtils::Exist(m_sStatusFilePath)) {
        if (-1 == unlink(m_sStatusFilePath.c_str())) {
            LOGEFUN << "simple checkpoint rm status file failed.";
            return false;
        }
    }

    return true;
}

bool SimpleCheckpoint::create_completed_status() {
    int fd = utils::FileUtils::Open(m_sStatusFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        return false;
    } else {
        close(fd);
        return true;
    }
}
}
}
