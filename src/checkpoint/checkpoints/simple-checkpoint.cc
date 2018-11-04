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
#include "../../utils/common-utils.h"

namespace flyingkv {
namespace checkpoint {
SimpleCheckpoint::SimpleCheckpoint(const std::string &rootDir, common::EntryCreateHandler &&ech) :
        m_sRootDir(rootDir), m_entryCreator(std::move(ech)) {

    m_sRootDir = rootDir;
    m_sCpFilePath = m_sRootDir + "/" + SMCP_PREFIX_NAME;
    m_sNewCpFilePath = m_sCpFilePath + SMCP_NEW_FILE_SUFFIX;
    m_sNewCpSaveOkFilePath = m_sRootDir + "/" + SMCP_COMPLETE_FLAG;
    m_sCpMetaFilePath = m_sRootDir + "/" + SMCP_META_SUFFIX;
    m_sNewCpMetaFilePath = m_sCpMetaFilePath + SMCP_NEW_FILE_SUFFIX;
    m_sNewStartFlagFilePath = m_sRootDir + "/" + SMCP_SAVE_START_FLAG;
}

CheckpointResult SimpleCheckpoint::Init() {
    common::ObjReleaseHandler<bool> handler(&m_bInited, [](bool *inited) {
        *inited = true;
    });
    if (-1 == utils::FileUtils::CreateDirPath(m_sRootDir, 0775)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " create dir " << m_sRootDir << " failed.";
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return check_and_recover();
}

// TODO(sunchao): 1. 把load定义一个模式抽象一下？免得和log的load逻辑重复
//                2. 同wal, io和计算异步？
LoadCheckpointResult SimpleCheckpoint::Load(EntryLoadedCallback callback) {
    if (UNLIKELY(!m_bInited)) {
        LOGEFUN << SMCP_NAME << UninitializedError;
        return LoadCheckpointResult(Code::Uninited, UninitializedError);
    }
    auto metaRs = load_meta();
    if (Code::OK != metaRs.Rc) {
        return LoadCheckpointResult(metaRs.Rc, metaRs.Errmsg);
    }
    if (0 == metaRs.EntryId) {    // 不存在checkpoint
        if (!utils::FileUtils::Exist(m_sCpFilePath)) {
            LOGIFUN << SMCP_NAME << " no checkpoint";
            return LoadCheckpointResult(SMCP_INVALID_ENTRY_ID);
        } else {
            SMCPLOGEFUN << " checkpoint meta file missing";
            return LoadCheckpointResult(Code::MissingFile, MissingFileError);
        }
    }

    if (!utils::FileUtils::Exist(m_sCpFilePath)) {
        SMCPLOGEFUN << " checkpoint file missing";
        return LoadCheckpointResult(Code::MissingFile, MissingFileError);
    }

    auto fd = utils::FileUtils::Open(m_sCpFilePath, O_RDONLY, 0, 0);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " open checkpoint file " << m_sCpFilePath << " error" << errmsg;
        return LoadCheckpointResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    // 1. check if file is empty.
    auto fileSize = utils::FileUtils::GetFileSize(fd);
    if (-1 == fileSize) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " get file size for " << m_sCpFilePath << " error " << errmsg;
        return LoadCheckpointResult(Code::FileCorrupt, errmsg);
    }
    if (0 == fileSize) {
        LOGDFUN4(SMCP_NAME, " file ", m_sCpFilePath, " is empty!");
        return LoadCheckpointResult(SMCP_INVALID_ENTRY_ID);
    }

    // 2. check if file header is corrupt.
    if (fileSize < SMCP_MAGIC_NO_LEN) {
        SMCPLOGEFUN << " file " << m_sCpFilePath << " header is corrupt!";
        return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
    }

    auto headerMpo = common::g_pMemPool->Get(SMCP_MAGIC_NO_LEN);
    auto headerBuffer = (uchar*)(headerMpo->Pointer());
    auto nRead = utils::IOUtils::ReadFully_V4(fd, (char*)headerBuffer, SMCP_MAGIC_NO_LEN);
    if (-1 == nRead) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " read " << m_sCpFilePath << " error " << errmsg;
        return LoadCheckpointResult(Code::FileSystemError, errmsg);
    }

    uint32_t magicNo = ByteOrderUtils::ReadUInt32(headerBuffer);
    if (SMCP_MAGIC_NO != magicNo) {
        SMCPLOGEFUN << m_sCpFilePath << " header is corrupt!";
        return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
    }
    headerMpo->Put();

    // 3. load checkpoint content
    auto cpEntryOffset = uint32_t(SMCP_MAGIC_NO_LEN);
    sys::MemPool::MemObject *pLastMpo = nullptr;
    uchar *lastBuffer = nullptr;
    uint32_t lastLeftSize = 0;
    bool ok = true;
    while (ok) {
        auto readSize = SMCP_READ_BATCH_SIZE + lastLeftSize;
        auto mpo = common::g_pMemPool->Get(readSize);
        auto buffer = (uchar*)(mpo->Pointer());
        if (lastBuffer) {
            auto lastOffset = SMCP_READ_BATCH_SIZE - lastLeftSize;
            memcpy(buffer, lastBuffer + lastOffset, lastLeftSize);
            lastBuffer = nullptr;
            pLastMpo->Put();
            pLastMpo = nullptr;
            lastLeftSize = 0;
        }

        nRead = utils::IOUtils::ReadFully_V4(fd, (char*)buffer + lastLeftSize, SMCP_READ_BATCH_SIZE);
        if (-1 == nRead) {
            mpo->Put();
            auto errmsg = strerror(errno);
            SMCPLOGEFUN << " read checkpoint " << m_sCpFilePath << " error " << errmsg;
            return LoadCheckpointResult(Code::FileSystemError, errmsg);
        }

        if (0 == nRead) {
            mpo->Put();
            if (0 != lastLeftSize) {
                SMCPLOGEFUN << " checkpoint file " << m_sCpFilePath << "corrupt";
                return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
            }
            return LoadCheckpointResult(metaRs.EntryId);
        }

        std::vector<common::IEntry*> entries;
        ok = (nRead == SMCP_READ_BATCH_SIZE);
        auto availableBufferSize = nRead + lastLeftSize;
        uint32_t offset = 0;
        while (offset < availableBufferSize) {
            auto leftSize = availableBufferSize - offset;
            // 剩下的不够一个entry了
            if (leftSize < (SMCP_SIZE_LEN + SMCP_VERSION_LEN)) {
                lastBuffer = buffer;
                pLastMpo = mpo;
                lastLeftSize = uint32_t(leftSize);
                break;
            }

            auto len = ByteOrderUtils::ReadUInt32(buffer + offset);
            auto walEntryLen = SMCP_ENTRY_EXTRA_FIELDS_SIZE + len;
            // 剩下的不够一个entry了
            if (leftSize < walEntryLen) {
                lastBuffer = buffer;
                pLastMpo = mpo;
                lastLeftSize = uint32_t(leftSize);
                break;
            }

            auto startPosOffset = SMCP_SIZE_LEN + SMCP_VERSION_LEN + len;
            auto startPos = ByteOrderUtils::ReadUInt32(buffer + offset + startPosOffset);
            if (startPos != cpEntryOffset) {
                mpo->Put();
                SMCPLOGEFUN << " parse checkpoint entry in " << m_sCpFilePath << " failed at offset " << offset;
                return LoadCheckpointResult(Code::FileCorrupt, FileCorruptError);
            }

            // auto version = *(buffer + offset + SMCP_SIZE_LEN);
            auto contentStartPtr = buffer + offset + SMCP_CONTENT_OFFSET;
            auto contentEndPtr = buffer + len - 1;
            common::Buffer b;
            b.Refresh(contentStartPtr, contentEndPtr, contentStartPtr, contentEndPtr, nullptr);
            auto entry = m_entryCreator();
            if (!entry->Decode(b)) {
                mpo->Put();
                SMCPLOGEFUN << " decode entry at offset " << offset << " failed!";
                return LoadCheckpointResult(Code::DecodeEntryError, DecodeEntryError);
            }

            entries.push_back(entry);
            auto forwardOffset = len + SMCP_ENTRY_EXTRA_FIELDS_SIZE;
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

// TODO(sunchao): 1.有时间优化为batch写 2. io和计算异步？
CheckpointResult SimpleCheckpoint::Save(IEntriesTraveller *traveller) {
    if (UNLIKELY(!m_bInited)) {
        LOGEFUN << SMCP_NAME << UninitializedError;
        return CheckpointResult(Code::Uninited, UninitializedError);
    }
    if (traveller->Empty()) {
        return CheckpointResult(Code::OK);
    }

    auto rs = check_and_recover();
    if (Code::OK != rs.Rc) {
        return rs;
    }

    auto maxEntryId = traveller->MaxId();
    rs = init_new_checkpoint(maxEntryId);
    if (Code::OK != rs.Rc) {
        return rs;
    }

    // save entries
        save_new_checkpoint(traveller);

    // create ok flag
    rs = create_ok_flag();
    if (Code::OK != rs.Rc) {
        return rs;
    }

    // rm start flag
    if (-1 == utils::FileUtils::Unlink(m_sNewStartFlagFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink start flag file " << m_sNewStartFlagFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    // replace meta and checkpoint
    rs = replace_old_checkpoint();
    if (Code::OK != rs.Rc) {
        return rs;
    }

    // clean ok flag
    if (-1 == utils::FileUtils::Unlink(m_sNewCpSaveOkFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink ok flag file " << m_sNewCpSaveOkFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult SimpleCheckpoint::init_new_checkpoint(uint64_t id) {
    LOGDFUN1("init new checkpoint");
    // create start flag
    auto fd = utils::FileUtils::Open(m_sNewStartFlagFilePath, O_RDONLY, O_CREAT, 644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " create start flag file " << m_sNewStartFlagFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    auto rs = create_new_meta_file(id);
    if (Code::OK != rs.Rc) {
        return rs;
    }

    return create_new_checkpoint_file();
}

bool SimpleCheckpoint::is_completed() {
    return !utils::FileUtils::Exist(m_sNewStartFlagFilePath) && !utils::FileUtils::Exist(m_sNewCpSaveOkFilePath);
}

CheckpointResult SimpleCheckpoint::create_ok_flag() {
    int fd = utils::FileUtils::Open(m_sNewCpSaveOkFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " create ok file " << m_sNewCpSaveOkFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    } else {
        if (-1 == close(fd)) {
            auto errmsg = strerror(errno);
            SMCPLOGEFUN << " close ok file " << m_sNewCpSaveOkFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }

        return CheckpointResult(Code::OK);
    }
}

CheckpointResult SimpleCheckpoint::check_and_recover() {
    if (is_completed()) {
        return CheckpointResult(Code::OK);
    }

    // case 1: 之前已完成checkpoint
    if (utils::FileUtils::Exist(m_sNewCpSaveOkFilePath)) {
        auto rs = replace_old_checkpoint();
        if (Code::OK != rs.Rc) {
            return rs;
        }

        if (-1 != unlink(m_sNewCpSaveOkFilePath.c_str())) {
            auto errmsg = strerror(errno);
            SMCPLOGEFUN << " unlink " << m_sNewCpSaveOkFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }

        return CheckpointResult(Code::OK);
    }

    // case 2: 之前未完成checkpoint
    return clean_new_cp_tmp();
}

CheckpointResult SimpleCheckpoint::clean_new_cp_tmp() {
    if (-1 == utils::FileUtils::Unlink(m_sNewCpMetaFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink " << m_sNewCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sNewCpFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink " << m_sNewCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sNewStartFlagFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink " << m_sNewStartFlagFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sNewCpSaveOkFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink " << m_sNewCpSaveOkFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult SimpleCheckpoint::replace_old_checkpoint() {
    if (-1 == utils::FileUtils::Unlink(m_sCpMetaFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink file " << m_sCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == utils::FileUtils::Unlink(m_sCpFilePath)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " unlink file " << m_sCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == rename(m_sNewCpMetaFilePath.c_str(), m_sCpMetaFilePath.c_str())) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " rename file " << m_sNewCpMetaFilePath << " to " << m_sCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    if (-1 == rename(m_sNewCpFilePath.c_str(), m_sCpFilePath.c_str())) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " rename file " << m_sNewCpFilePath << " to " << m_sCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

SimpleCheckpoint::LoadMetaResult SimpleCheckpoint::load_meta() {
    if (!utils::FileUtils::Exist(m_sCpMetaFilePath)) {
        return LoadMetaResult(SMCP_INVALID_ENTRY_ID);
    }

    auto entryIdStr = utils::FileUtils::ReadAllString(m_sCpMetaFilePath);
    auto entryId = utils::CommonUtils::ToInteger<uint64_t>(entryIdStr);
    if (SMCP_INVALID_ENTRY_ID == entryId) {
        SMCPLOGEFUN << " load entry id from " << m_sCpMetaFilePath << " error";
        return LoadMetaResult(Code::FileCorrupt, FileCorruptError);
    }

    return LoadMetaResult(entryId);
}

CheckpointResult SimpleCheckpoint::create_new_meta_file(uint64_t id) {
    LOGDFUN2("create sm checkpoint meta file ", m_sNewCpMetaFilePath.c_str());
    int fd = utils::FileUtils::Open(m_sNewCpMetaFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << "create new meta file " << m_sNewCpMetaFilePath.c_str() << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    auto idStr = utils::CommonUtils::ToString(id);
    if (-1 == utils::IOUtils::WriteFully(fd, idStr.c_str(), idStr.size())) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " write new meta file " << m_sNewCpMetaFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult SimpleCheckpoint::create_new_checkpoint_file() {
    int fd = utils::FileUtils::Open(m_sNewCpFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LOGFFUN << "create new checkpoint file " << m_sNewCpFilePath.c_str() << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    // write file header.
    uchar header[SMCP_MAGIC_NO_LEN];
    ByteOrderUtils::WriteUInt32(header, SMCP_MAGIC_NO);
    if (-1 == utils::IOUtils::WriteFully(fd, (char*)header, SMCP_MAGIC_NO_LEN)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " write file " << m_sNewCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }

    return CheckpointResult(Code::OK);
}

CheckpointResult SimpleCheckpoint::save_new_checkpoint(IEntriesTraveller *traveller) {
    auto fd = utils::FileUtils::Open(m_sNewCpFilePath, O_WRONLY, 0, 0);
    common::FileCloser fc(fd);
    common::IEntry *entry = nullptr;
    uint32_t offset = SMCP_MAGIC_NO_LEN;
    if (-1 == lseek(fd, SMCP_MAGIC_NO_LEN, SEEK_SET)) {
        auto errmsg = strerror(errno);
        SMCPLOGEFUN << " lseek file " << m_sNewCpFilePath << " error " << errmsg;
        return CheckpointResult(Code::FileSystemError, errmsg);
    }
    while ((entry = traveller->GetNextEntry())) {
        std::shared_ptr<common::Buffer> eb;
        if (UNLIKELY(!entry->Encode(eb))) {
            SMCPLOGEFUN << " encode entry error";
            return CheckpointResult(Code::EncodeEntryError, EncodeEntryError);
        }

        auto rawEntrySize = uint32_t(eb->AvailableLength());
        auto walEntrySize = rawEntrySize + SMCP_ENTRY_EXTRA_FIELDS_SIZE;
        // mpo will be Putted in Buffer 'b'.
        auto mpo = common::g_pMemPool->Get(walEntrySize);
        auto bufferStart = (uchar*)(mpo->Pointer());
        common::Buffer wb;
        wb.Refresh(bufferStart, bufferStart + walEntrySize - 1, bufferStart, bufferStart + walEntrySize - 1, mpo);
        // size
        ByteOrderUtils::WriteUInt32(wb.GetPos(), rawEntrySize);
        wb.MoveHeadBack(SMCP_SIZE_LEN);

        // version
        *wb.GetPos() = SMCP_VERSION;
        wb.MoveHeadBack(SMCP_VERSION_LEN);

        // content
        memcpy(eb->GetPos(), wb.GetPos(), size_t(rawEntrySize));
        wb.MoveHeadBack(rawEntrySize);

        // entry offset
        ByteOrderUtils::WriteUInt32(wb.GetPos(), offset);
        if (-1 == utils::IOUtils::WriteFully(fd, (char*)(wb.GetStart()), walEntrySize)) {
            auto errmsg = strerror(errno);
            SMCPLOGEFUN << "write file " << m_sNewCpFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }
        if (-1 == fdatasync(fd)) {
            auto errmsg = strerror(errno);
            SMCPLOGEFUN << " fdatasync file " << m_sNewCpFilePath << " error " << errmsg;
            return CheckpointResult(Code::FileSystemError, errmsg);
        }
        offset += walEntrySize;
    }

    return CheckpointResult(Code::OK);
}
}
}
