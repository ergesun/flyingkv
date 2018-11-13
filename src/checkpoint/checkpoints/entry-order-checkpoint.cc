/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <sys/mman.h>
#include <wait.h>
#include "../../common/common-def.h"
#include "../../utils/file-utils.h"
#include "../../utils/io-utils.h"
#include "../../utils/codec-utils.h"

#include "../ientries-traveller.h"

#include "entry-order-checkpoint.h"
#include "../../common/buffer.h"
#include "../../common/global-vars.h"
#include "../../utils/common-utils.h"

namespace flyingkv {
namespace checkpoint {
EntryOrderCheckpoint::EntryOrderCheckpoint(const EntryOrderCheckpointConfig *pc) :
        m_sRootDir(pc->RootDirPath), m_entryCreator(pc->ECH), m_writeEntryVersion(pc->WriteEntryVersion),
        m_batchReadSize(pc->BatchReadSize) {
    m_sCpFilePath = m_sRootDir + "/" + EOCP_PREFIX_NAME;
    m_sNewCpFilePath = m_sCpFilePath + EOCP_NEW_FILE_SUFFIX;
    m_sNewCpSaveOkFilePath = m_sRootDir + "/" + EOCP_COMPLETE_FLAG;
    m_sCpMetaFilePath = m_sRootDir + "/" + EOCP_META_SUFFIX;
    m_sNewCpMetaFilePath = m_sCpMetaFilePath + EOCP_NEW_FILE_SUFFIX;
    m_sNewStartFlagFilePath = m_sRootDir + "/" + EOCP_SAVE_START_FLAG;
    m_sLockPath = m_sRootDir + "/" + EOCP_LOCK_NAME;
}

CheckpointResult EntryOrderCheckpoint::Init() {
    LOGDTAG;
    common::ObjReleaseHandler<bool> handler(&m_bInited, [](bool *inited) {
        *inited = true;
    });
    if (-1 == utils::FileUtils::CreateDirPath(m_sRootDir, 0775)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " create dir " << m_sRootDir << " failed.";
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return check_and_recover();
}

// TODO(sunchao): 1. 把load定义一个模式抽象一下？免得和log的load逻辑重复
//                2. 同wal, io和计算异步？
LoadCheckpointResult EntryOrderCheckpoint::Load(EntryLoadedCallback callback) {
    LOGDTAG;
    if (UNLIKELY(!m_bInited)) {
        LOGEFUN << EOCP_NAME << UninitializedError;
        return LoadCheckpointResult(Code::Uninited, UninitializedError);
    }
    auto metaRs = load_meta();
    if (Code::OK != metaRs.Rc) {
        return LoadCheckpointResult(metaRs.Rc, metaRs.Errmsg);
    }

    if (metaRs.EntryId == EOCP_INVALID_ENTRY_ID) {
        return LoadCheckpointResult(EOCP_INVALID_ENTRY_ID);
    }

    if (!utils::FileUtils::Exist(m_sCpFilePath)) {
        EOCPLOGEFUN << " checkpoint file missing";
        return LoadCheckpointResult(Code::MissingFile, MissingFileError);
    }

    auto fd = utils::FileUtils::Open(m_sCpFilePath, O_RDONLY, 0, 0);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " open checkpoint file " << m_sCpFilePath << " error" << errmsg;
        return LoadCheckpointResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    auto fileSize = utils::FileUtils::GetFileSize(fd);
    if (-1 == fileSize) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " get file size for " << m_sCpFilePath << " error " << errmsg;
        return LoadCheckpointResult(Code::FileCorrupt, errmsg);
    }

    // check if file header is corrupt.
    if (fileSize < EOCP_MAGIC_NO_LEN) {
        EOCPLOGEFUN << " file " << m_sCpFilePath << " header is corrupt!";
        return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
    }

    auto headerMpo = common::g_pMemPool->Get(EOCP_MAGIC_NO_LEN);
    auto headerBuffer = (uchar*)(headerMpo->Pointer());
    auto nRead = utils::IOUtils::ReadFully_V4(fd, (char*)headerBuffer, EOCP_MAGIC_NO_LEN);
    if (-1 == nRead) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " read " << m_sCpFilePath << " error " << errmsg;
        return LoadCheckpointResult(Code::FileSystemError, errmsg);
    }

    uint32_t magicNo = ByteOrderUtils::ReadUInt32(headerBuffer);
    if (EOCP_MAGIC_NO != magicNo) {
        EOCPLOGEFUN << m_sCpFilePath << " header is corrupt!";
        return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
    }
    headerMpo->Put();

    // load checkpoint content
    auto cpEntryOffset = uint32_t(EOCP_MAGIC_NO_LEN);
    sys::MemPool::MemObject *pLastMpo = nullptr;
    uchar *lastBuffer = nullptr;
    uint32_t lastLeftSize = 0;
    uint32_t lastOffset = 0;
    bool ok = true;
    while (ok) {
        auto readSize = m_batchReadSize + lastLeftSize;
        auto mpo = common::g_pMemPool->Get(readSize);
        auto buffer = (uchar*)(mpo->Pointer());
        if (lastBuffer) {
            memcpy(buffer, lastBuffer + lastOffset, lastLeftSize);
            lastOffset = 0;
            lastBuffer = nullptr;
            pLastMpo->Put();
            pLastMpo = nullptr;
        }

        nRead = utils::IOUtils::ReadFully_V4(fd, (char*)buffer + lastLeftSize, m_batchReadSize);
        if (-1 == nRead) {
            mpo->Put();
            auto errmsg = strerror(errno);
            EOCPLOGEFUN << " read checkpoint " << m_sCpFilePath << " error " << errmsg;
            return LoadCheckpointResult(Code::FileSystemError, errmsg);
        }

        if (0 == nRead) {
            mpo->Put();
            if (0 != lastLeftSize) {
                EOCPLOGEFUN << " checkpoint file " << m_sCpFilePath << "corrupt";
                return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
            }
            return LoadCheckpointResult(metaRs.EntryId);
        }

        std::vector<common::IEntry*> entries;
        ok = (nRead == m_batchReadSize);
        auto availableBufferSize = nRead + lastLeftSize;
        lastLeftSize = 0; // 旧的buffer + 新读取的buffer开始处理了，上一次遗留的size就无用了。
        uint32_t offset = 0;
        while (offset < availableBufferSize) {
            auto leftSize = availableBufferSize - offset;
            uint32_t len = 0;
            bool notEnoughOneEntry = false;
            if (leftSize < EOCP_ENTRY_HEADER_LEN) {
                notEnoughOneEntry = true;
            } else {
                len = ByteOrderUtils::ReadUInt32(buffer + offset);
                // 剩下的不够一个entry了
                if (leftSize < EOCP_ENTRY_EXTRA_FIELDS_SIZE + len) {
                    notEnoughOneEntry = true;
                }
            }

            if (notEnoughOneEntry) { // 剩下的不够一个entry了
                if (!ok) { // 文件读尽了
                    mpo->Put();
                    EOCPLOGEFUN << " parse entry in " << m_sCpFilePath << " failed at offset " << offset;
                    return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
                }
                lastBuffer = buffer;
                pLastMpo = mpo;
                lastLeftSize = uint32_t(leftSize);
                lastOffset = offset;
                break;
            }

            auto startPosOffset = EOCP_SIZE_LEN + EOCP_VERSION_LEN + len;
            auto startPos = ByteOrderUtils::ReadUInt32(buffer + offset + startPosOffset);
            if (startPos != cpEntryOffset) {
                mpo->Put();
                EOCPLOGEFUN << " parse checkpoint entry in " << m_sCpFilePath << " failed at offset " << offset;
                return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
            }

            // auto version = *(buffer + offset + SMCP_SIZE_LEN);
            auto contentStartPtr = buffer + offset + EOCP_CONTENT_OFFSET;
            auto contentEndPtr = contentStartPtr + len - 1;
            common::Buffer b;
            b.Refresh(contentStartPtr, contentEndPtr, contentStartPtr, contentEndPtr, nullptr, false);
            auto entry = m_entryCreator(b);
            if (!entry->Decode(b)) {
                mpo->Put();
                EOCPLOGEFUN << " decode entry at offset " << offset << " failed!";
                return LoadCheckpointResult(Code::DecodeEntryError, DecodeEntryError);
            }

            entries.push_back(entry);
            auto forwardOffset = len + EOCP_ENTRY_EXTRA_FIELDS_SIZE;
            offset += forwardOffset;
            cpEntryOffset += forwardOffset;
        }

        if (!pLastMpo) {
            mpo->Put();
        }
        callback(entries);
    }

    return LoadCheckpointResult(metaRs.EntryId);
}

SaveCheckpointResult EntryOrderCheckpoint::Save(IEntriesTraveller *traveller) {
    LOGDTAG;
    if (UNLIKELY(!m_bInited)) {
        LOGEFUN << EOCP_NAME << UninitializedError;
        return SaveCheckpointResult(Code::Uninited, UninitializedError);
    }

    auto lockRs = utils::FileUtils::IsLocking(m_sLockPath);
    if (-1 == lockRs) {
        auto errmsg  = strerror(errno);
        LOGEFUN << "lock file " << m_sLockPath << " error " << errmsg;
        return SaveCheckpointResult(Code::FileSystemError, errmsg);
    }

    if (1 == lockRs) {
        return SaveCheckpointResult(Code::Locking, IsLockingError);
    }

    traveller->Prepare();
    auto pid = fork();
    if (pid < 0) {
        auto errmsg = strerror(errno);
        LOGEFUN << "fork process error " << errmsg;
        return SaveCheckpointResult(Code::ForkError, errmsg);
    } else if (pid == 0) {
        do_save_in_child(traveller);
        LOGIFUN << "checkpoint child process exit.";
        exit(0);
    } else {
        traveller->CompletePrepare();
        return do_save_in_parent(pid);
    }
}

SaveCheckpointResult EntryOrderCheckpoint::do_save_in_parent(int childPid) {
    LOGITAG;
    while (true) {
        auto pid = waitpid(childPid, nullptr, 0);
        if (-1 == pid) {
            auto errmsg = strerror(errno);
            if (EINTR == errno) {
                LOGEFUN << "wait child error " << errmsg;
                continue;
            } else {
                return SaveCheckpointResult(Code::WaitChildError, errmsg);
            }
        } else {
            if (is_completed()) {
                auto rs = load_meta();
                if (Code::OK != rs.Rc) {
                    return SaveCheckpointResult(rs.Rc, rs.Errmsg);
                }
                return SaveCheckpointResult(rs.EntryId);
            } else {
                return SaveCheckpointResult(Code::UnknownChildProcessError, UnknownChildError);
            }
        }
    }
}

// TODO(sunchao): 1.有时间优化为batch写 2. io和计算异步？
void EntryOrderCheckpoint::do_save_in_child(IEntriesTraveller *traveller) {
    auto lockFile = utils::FileUtils::LockPath(m_sLockPath);
    if (-1 == lockFile.fd) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " lock file " << m_sLockPath << " error " << errmsg;
        exit(1);
    }
    auto rs = check_and_recover();
    if (Code::OK != rs.Rc) {
        return;
    }

    auto maxEntryId = traveller->MaxId();
    rs = init_new_checkpoint(maxEntryId);
    if (Code::OK != rs.Rc) {
        return;
    }

    // save entries
    save_new_checkpoint(traveller);

    // create ok flag
    rs = create_ok_flag();
    if (Code::OK != rs.Rc) {
        return;
    }

    // close and rm lock file
    if (-1 == utils::FileUtils::CloseFile(lockFile)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " close lock file " << lockFile.path << " error " << errmsg;
        return;
    }

    if (-1 == utils::FileUtils::Unlink(lockFile.path)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink lock file " << lockFile.path << " error " << errmsg;
        return;
    }

    // rm start flag
    if (-1 == utils::FileUtils::Unlink(m_sNewStartFlagFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink start flag file " << m_sNewStartFlagFilePath << " error " << errmsg;
        return;
    }

    // replace meta and checkpoint
    rs = replace_old_checkpoint();
    if (Code::OK != rs.Rc) {
        return;
    }

    // clean ok flag
    if (-1 == utils::FileUtils::Unlink(m_sNewCpSaveOkFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink ok flag file " << m_sNewCpSaveOkFilePath << " error " << errmsg;
    }
}

CheckpointResult EntryOrderCheckpoint::init_new_checkpoint(uint64_t id) {
    LOGDFUN1("init new checkpoint");
    // create start flag
    auto fd = utils::FileUtils::Open(m_sNewStartFlagFilePath, O_WRONLY, O_CREAT, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " create start flag file " << m_sNewStartFlagFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    close(fd);
    auto rs = create_new_meta_file(id);
    if (Code::OK != rs.Rc) {
        return rs;
    }

    return create_new_checkpoint_file();
}

bool EntryOrderCheckpoint::is_completed() {
    return !utils::FileUtils::Exist(m_sLockPath) && !utils::FileUtils::Exist(m_sNewStartFlagFilePath) && !utils::FileUtils::Exist(m_sNewCpSaveOkFilePath);
}

CheckpointResult EntryOrderCheckpoint::create_ok_flag() {
    int fd = utils::FileUtils::Open(m_sNewCpSaveOkFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " create ok file " << m_sNewCpSaveOkFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    } else {
        if (-1 == close(fd)) {
            auto errmsg = strerror(errno);
            EOCPLOGEFUN << " close ok file " << m_sNewCpSaveOkFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }

        return CheckpointResult(Code::OK);
    }
}

CheckpointResult EntryOrderCheckpoint::check_and_recover() {
    if (is_completed()) {
        return CheckpointResult(Code::OK);
    }

    // case 1: 之前已完成checkpoint
    if (utils::FileUtils::Exist(m_sNewCpSaveOkFilePath)) {
        auto rs = replace_old_checkpoint();
        if (Code::OK != rs.Rc) {
            return rs;
        }

        if (-1 == unlink(m_sNewCpSaveOkFilePath.c_str())) {
            auto errmsg = strerror(errno);
            EOCPLOGEFUN << " unlink " << m_sNewCpSaveOkFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }

        return CheckpointResult(Code::OK);
    }

    // case 2: 之前未完成checkpoint
    return clean_new_cp_tmp();
}

CheckpointResult EntryOrderCheckpoint::clean_new_cp_tmp() {
    if (-1 == utils::FileUtils::Unlink(m_sNewCpMetaFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink " << m_sNewCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sNewCpFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink " << m_sNewCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sNewStartFlagFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink " << m_sNewStartFlagFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sNewCpSaveOkFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink " << m_sNewCpSaveOkFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult EntryOrderCheckpoint::replace_old_checkpoint() {
    if (-1 == utils::FileUtils::Unlink(m_sCpMetaFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink file " << m_sCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sCpFilePath)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " unlink file " << m_sCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == rename(m_sNewCpMetaFilePath.c_str(), m_sCpMetaFilePath.c_str())) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " rename file " << m_sNewCpMetaFilePath << " to " << m_sCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == rename(m_sNewCpFilePath.c_str(), m_sCpFilePath.c_str())) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " rename file " << m_sNewCpFilePath << " to " << m_sCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

EntryOrderCheckpoint::LoadMetaResult EntryOrderCheckpoint::load_meta() {
    if (!utils::FileUtils::Exist(m_sCpMetaFilePath)) {
        LOGIFUN << "file " << m_sCpMetaFilePath << " not exist";
        return LoadMetaResult(EOCP_INVALID_ENTRY_ID);
    }

    auto entryIdStr = utils::FileUtils::ReadAllString(m_sCpMetaFilePath);
    uint64_t entryId;
    auto rs = utils::CommonUtils::ToUint46_t(entryIdStr, entryId);
    if (!rs) {
        EOCPLOGEFUN << " load entry id from " << m_sCpMetaFilePath << " error";
        return LoadMetaResult(Code::FileCorrupt, FileCorruptError);
    }

    return LoadMetaResult(entryId);
}

CheckpointResult EntryOrderCheckpoint::create_new_meta_file(uint64_t id) {
    LOGDFUN2("create sm checkpoint meta file ", m_sNewCpMetaFilePath.c_str());
    int fd = utils::FileUtils::Open(m_sNewCpMetaFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << "create new meta file " << m_sNewCpMetaFilePath.c_str() << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    auto idStr = utils::CommonUtils::ToString(id);
    if (-1 == utils::IOUtils::WriteFully(fd, idStr.c_str(), idStr.size())) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " write new meta file " << m_sNewCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult EntryOrderCheckpoint::create_new_checkpoint_file() {
    int fd = utils::FileUtils::Open(m_sNewCpFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LOGFFUN << "create new checkpoint file " << m_sNewCpFilePath.c_str() << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    // write file header.
    uchar header[EOCP_MAGIC_NO_LEN];
    ByteOrderUtils::WriteUInt32(header, EOCP_MAGIC_NO);
    if (-1 == utils::IOUtils::WriteFully(fd, (char*)header, EOCP_MAGIC_NO_LEN)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " write file " << m_sNewCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult EntryOrderCheckpoint::save_new_checkpoint(IEntriesTraveller *traveller) {
    auto fd = utils::FileUtils::Open(m_sNewCpFilePath, O_WRONLY, 0, 0);
    common::FileCloser fc(fd);
    common::IEntry *entry = nullptr;
    uint32_t offset = EOCP_MAGIC_NO_LEN;
    if (-1 == lseek(fd, EOCP_MAGIC_NO_LEN, SEEK_SET)) {
        auto errmsg = strerror(errno);
        EOCPLOGEFUN << " lseek file " << m_sNewCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }
    while ((entry = traveller->GetNextEntry())) {
        std::shared_ptr<common::Buffer> eb;
        if (UNLIKELY(!entry->Encode(eb))) {
            EOCPLOGEFUN << " encode entry error";
            return CheckpointResult(Code::EncodeEntryError, EncodeEntryError);
        }

        auto rawEntrySize = uint32_t(eb->AvailableLength());
        auto walEntrySize = rawEntrySize + EOCP_ENTRY_EXTRA_FIELDS_SIZE;
        // mpo will be Putted in Buffer 'b'.
        auto mpo = common::g_pMemPool->Get(walEntrySize);
        auto bufferStart = (uchar*)(mpo->Pointer());
        common::Buffer wb;
        wb.Refresh(bufferStart, bufferStart + walEntrySize - 1, bufferStart, bufferStart + walEntrySize - 1, mpo, true);
        // size
        ByteOrderUtils::WriteUInt32(wb.GetPos(), rawEntrySize);
        wb.MoveHeadBack(EOCP_SIZE_LEN);

        // version
        *wb.GetPos() = m_writeEntryVersion;
        wb.MoveHeadBack(EOCP_VERSION_LEN);

        // content
        if (eb->Valid()) {
            memcpy(wb.GetPos(), eb->GetPos(), size_t(rawEntrySize));
            wb.MoveHeadBack(rawEntrySize);
        }

        // entry offset
        ByteOrderUtils::WriteUInt32(wb.GetPos(), offset);
        if (-1 == utils::IOUtils::WriteFully(fd, (char*)(wb.GetStart()), walEntrySize)) {
            auto errmsg = strerror(errno);
            EOCPLOGEFUN << "write file " << m_sNewCpFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }
        if (-1 == fdatasync(fd)) {
            auto errmsg = strerror(errno);
            EOCPLOGEFUN << " fdatasync file " << m_sNewCpFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }
        offset += walEntrySize;
    }

    return CheckpointResult(Code::OK);
}
}
}
