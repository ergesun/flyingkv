/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <memory>
#include <unistd.h>
#include <sys/mman.h>
#include <algorithm>

#include "../../common/common-def.h"

#include "../../common/ientry.h"

#include "log-clean-wal.h"
#include "../../utils/file-utils.h"
#include "../../utils/io-utils.h"
#include "../../utils/codec-utils.h"
#include "../../utils/protobuf-utils.h"
#include "../../common/global-vars.h"
#include "../../common/buffer.h"
#include "../../utils/common-utils.h"

namespace flyingkv {
using namespace utils;
namespace wal {
LogCleanWal::LogCleanWal(const LogCleanWalConfig *pc) :
        m_sRootDir(pc->RootDirPath), m_entryCreator(pc->ECH), m_writeEntryVersion(pc->WriteEntryVersion),
        m_segmentMaxSize(pc->MaxSegmentSize), m_batchReadSize(pc->ReadBatchSize) {
    if (m_segmentMaxSize < LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE) {
        LOGFFUN << LOGCLEAN_WAL_NAME << " conf segment max size is too small, it at least cannot be smaller than "
                << LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;
    }

    if (m_batchReadSize < LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE) {
        LOGFFUN << LOGCLEAN_WAL_NAME << " conf batch read size is too small, it at least cannot be smaller than "
                << LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;
    }

    m_sTruncInfoFilePath = m_sRootDir + "/" + LOGCLEAN_WAL_TRUNC_META_NAME;
    m_sTruncOKFlagFilePath = m_sRootDir + "/" + LOGCLEAN_WAL_TRUNC_OK_NAME;
    m_maxEntrySize = m_batchReadSize - LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;
}

LogCleanWal::~LogCleanWal() {
    if (-1 != m_currentSegFd) {
        close(m_currentSegFd);
        m_currentSegFd = -1;
    }
}

/**
 * 清理未完成的trunc，初始化segment相关信息
 */
WalResult LogCleanWal::Init() {
    common::ObjReleaseHandler<bool> handler(&m_bInited, [](bool *inited) {
       *inited = true;
    });
    if (-1 == utils::FileUtils::CreateDirPath(m_sRootDir, 0775)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " create wal dir " << m_sRootDir.c_str() << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }
    auto maxTruncUncompletedSegId = get_uncompleted_trunc_segments_max_id();
    auto lsaRs = list_segments_asc();
    if (Code::OK != lsaRs.Rc) {
        return WalResult(lsaRs.Rc, lsaRs.Errmsg);
    }

    if (LOGCLEAN_WAL_INVALID_SEGMENT_ID != maxTruncUncompletedSegId) {
        if (lsaRs.Segments.empty()) { // 没有segment了,只是trunc info没来得及清理就挂了
            m_vInitSegments.clear();
            auto rs = clean_trunc_status();
            if (Code::OK != rs.Rc) {
                return WalResult(rs.Rc, rs.Errmsg);
            }
        }

        auto minSegmentId = lsaRs.Segments[0].Id;
        for (uint64_t i = minSegmentId; i <= maxTruncUncompletedSegId; ++i) {
            auto cleanSegFilePath = generate_segment_file_path(i);
            if (-1 == utils::FileUtils::Unlink(cleanSegFilePath)) {
                auto errmsg = strerror(errno);
                LCLOGEFUN << " clean segment " << cleanSegFilePath << " error " << errmsg;
                return WalResult(Code::FileSystemError, errmsg);
            }
        }

        lsaRs.Segments.erase(std::remove_if(lsaRs.Segments.begin(), lsaRs.Segments.end(),
                                      [maxTruncUncompletedSegId](const LogCleanWalSegmentFileInfo &s) {
                                          return s.Id <= maxTruncUncompletedSegId;
                                      }
                                      ),
                       lsaRs.Segments.end());
    }

    auto rs = clean_trunc_status();
    if (Code::OK != rs.Rc) {
        return WalResult(rs.Rc, rs.Errmsg);
    }
    if (lsaRs.Segments.empty()) {
        m_minSegmentId = 1; // 初始值
        m_currentSegmentId = LOGCLEAN_WAL_INVALID_ENTRY_ID;
        rs = create_new_segment_file();
        if (rs.Rc != Code::OK) {
            return rs;
        }
    } else {
        m_minSegmentId = lsaRs.Segments[0].Id;
        m_currentSegmentId = lsaRs.Segments[lsaRs.Segments.size() - 1].Id;
    }

    m_vInitSegments.swap(lsaRs.Segments);
    auto curSegmentFilePath = generate_segment_file_path(m_currentSegmentId);
    auto fd = utils::FileUtils::Open(curSegmentFilePath, O_WRONLY, O_CREAT, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << "open file " << curSegmentFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    m_currentSegFd = fd;
    m_currentSegFilePath = std::move(curSegmentFilePath);
    if (-1 == lseek((fd), utils::FileUtils::GetFileSize(fd), SEEK_SET)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " lseek file " << m_currentSegFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    return WalResult(Code::OK);
}

LoadResult LogCleanWal::Load(const WalEntryLoadedCallback &callback) {
    if (UNLIKELY(!m_bInited)) {
        LOGEFUN << LOGCLEAN_WAL_NAME << UninitializedError;
        return LoadResult(Code::Uninited, UninitializedError);
    }

    common::ObjReleaseHandler<bool> handler(&m_bLoaded, [](bool *loaded) {
        *loaded = true;
    });

    if (m_vInitSegments.empty()) {
        return LoadResult(Code::OK);
    }

    m_minSegmentId = m_vInitSegments[0].Id;
    auto maxIdx = m_vInitSegments.size() - 1;
    for (size_t i = 0; i <= maxIdx; ++i) {
        auto rs = load_segment(m_vInitSegments[i].Path, m_vInitSegments[i].Id, callback);
        if (Code::OK != rs.Rc) {
            return LoadResult(rs.Rc, rs.Errmsg);
        }
        if (i == maxIdx) {
            m_curSegFileSize = rs.FileSize;
        }
    }

    return LoadResult(Code::OK);
}

// TODO(sunchao): 抽象一个编码函数，对应不同的log version。目前这个只能编码version=1
AppendEntryResult LogCleanWal::AppendEntry(common::IEntry *entry) {
    if (UNLIKELY(!m_bInited)) {
        LCLOGEFUN << UninitializedError;
        return AppendEntryResult(Code::Uninited, UninitializedError);
    }
    if (UNLIKELY(!m_bLoaded)) {
        LCLOGEFUN << UnloadedError;
        return AppendEntryResult(Code::Unloaded, UnloadedError);
    }
    std::shared_ptr<common::Buffer> eb;
    if (UNLIKELY(!entry->Encode(eb))) {
        LCLOGEFUN << EncodeEntryError;
        return AppendEntryResult(Code::EncodeEntryError, EncodeEntryError);
    }

    auto walEntryStartOffset = m_curSegFileSize;
    auto rawEntrySize = uint32_t(eb->AvailableLength());
    auto walEntrySize = rawEntrySize + LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;
    if (walEntrySize > m_maxEntrySize) {
        LCLOGEFUN << " entry size cannot be larger than " << m_maxEntrySize << " bytes";
        return AppendEntryResult(Code::EntryBytesSizeOverflow, EntryBytesSizeOverflowError);
    }

    // mpo will be Putted in Buffer 'b'.
    auto mpo = common::g_pMemPool->Get(walEntrySize);
    auto bufferStart = (uchar*)(mpo->Pointer());
    common::Buffer wb;
    wb.Refresh(bufferStart, bufferStart + walEntrySize - 1, bufferStart, bufferStart + walEntrySize - 1, mpo, true);
    // size
    ByteOrderUtils::WriteUInt32(wb.GetPos(), rawEntrySize);
    wb.MoveHeadBack(LOGCLEAN_WAL_SIZE_LEN);

    // version
    *(wb.GetPos()) = m_writeEntryVersion;
    wb.MoveHeadBack(LOGCLEAN_WAL_VERSION_LEN);

    // log id
    ByteOrderUtils::WriteUInt64(wb.GetPos(), m_currentEntryIdx);
    wb.MoveHeadBack(LOGCLEAN_WAL_ENTRY_ID_LEN);

    // content
    if (eb->Valid()) {
        memcpy(wb.GetPos(), eb->GetPos(), size_t(rawEntrySize));
        wb.MoveHeadBack(rawEntrySize);
    }

    // offset
    ByteOrderUtils::WriteUInt32(wb.GetPos(), walEntryStartOffset);
    if (-1 == utils::IOUtils::WriteFully(m_currentSegFd, (char*)(wb.GetStart()), size_t(walEntrySize))) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " write file " << m_currentSegFilePath << " error " << errmsg;
        return AppendEntryResult(Code::FileSystemError, errmsg);
    }

    if (-1 == fdatasync(m_currentSegFd)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " fdatasync file " << m_currentSegFilePath << " error " << errmsg;
        return AppendEntryResult(Code::FileSystemError, errmsg);
    }
    m_curSegFileSize += walEntrySize;
    m_mpEntriesIdSegId[m_currentEntryIdx] = m_currentSegmentId;
    ++m_currentEntryIdx;
    if (m_curSegFileSize >= m_segmentMaxSize) {
        auto rs = create_new_segment_file();
        if (rs.Rc != Code::OK) {
            return AppendEntryResult(rs.Rc, rs.Errmsg);
        }
    }

    return AppendEntryResult(m_currentEntryIdx);
}

TruncateResult LogCleanWal::Truncate(uint64_t id) {
    if (UNLIKELY(!m_bInited)) {
        LOGEFUN << LOGCLEAN_WAL_NAME << UninitializedError;
        return TruncateResult(Code::Uninited, UninitializedError);
    }

    if (UNLIKELY(!m_bLoaded)) {
        LCLOGEFUN << " has not been loaded.";
        return TruncateResult(Code::Unloaded, UnloadedError);
    }

    auto iter = m_mpEntriesIdSegId.find(id);
    if (m_mpEntriesIdSegId.end() == iter) {
        LCLOGEFUN << " cannot find truncate entry id " << id;
        return TruncateResult(Code::InvalidEntryId, InvalidEntryIdError);
    }

    if (0 == m_curSegFileSize) {
        LOGIFUN << LOGCLEAN_WAL_NAME << " no entry, so no need to truncate.";
        return TruncateResult(Code::OK);
    }

    // 1. 写入truncate信息
    auto rs = create_trunc_info(id);
    if (Code::OK != rs.Rc) {
        return TruncateResult(rs.Rc, rs.Errmsg);
    }

    // 2. truncate
    auto maxTruncateSegId = iter->second;
    // 最后有内容的segment不做truncate动作以便保留entry id
    if (maxTruncateSegId == m_currentSegmentId) {
        --maxTruncateSegId;
    }

    // 看看后面的一个segment是否有内容
    auto fp = generate_segment_file_path(maxTruncateSegId + 1);
    auto nextSegSize = utils::FileUtils::GetFileSize(fp);
    if (-1 == nextSegSize) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " access file " << fp << " error " << errmsg;
        return TruncateResult(Code::FileSystemError, errmsg);
    }

    if (0 == nextSegSize) {
        --maxTruncateSegId;
    }

    uint64_t maxCleanEntryId = LOGCLEAN_WAL_INVALID_ENTRY_ID;
    for (uint64_t i = m_minSegmentId; i <= maxTruncateSegId; ++i) {
        fp = generate_segment_file_path(i);
        if (i == maxTruncateSegId) {
            auto lrs = load_segment_max_entry_id(fp);
            if (Code::OK != lrs.Rc) {
                return TruncateResult(lrs.Rc, lrs.Errmsg);
            }

            maxCleanEntryId = lrs.EntryId;
        }
        if (-1 == utils::FileUtils::Unlink(fp)) {
            auto errmsg = strerror(errno);
            LCLOGEFUN << " unlink segment " << fp << " error " << errmsg;
            return TruncateResult(Code::FileSystemError, errmsg);
        }
    }

    if (LOGCLEAN_WAL_INVALID_ENTRY_ID == maxCleanEntryId) {
        m_minSegmentId = LOGCLEAN_WAL_MIN_ENTRY_ID;
        return TruncateResult(Code::OK);
    }

    m_minSegmentId = maxTruncateSegId + 1;
    for (auto i = m_minSegmentId; i <= maxCleanEntryId; ++i) {
        auto iter = m_mpEntriesIdSegId.find(i);
        if (m_mpEntriesIdSegId.end() != iter) {
            m_mpEntriesIdSegId.erase(iter);
        }
    }

    auto wrs = create_trunc_ok_flag();
    if (Code::OK != wrs.Rc) {
        return TruncateResult(wrs.Rc, wrs.Errmsg);
    }

    auto crs = clean_trunc_status();
    if (Code::OK != crs.Rc) {
        return TruncateResult(crs.Rc, crs.Errmsg);
    }

    return TruncateResult(Code::OK);
}

// TODO(sunchao): 1. 抽象一个解析函数，对应不同的log version。目前这个只能解析version=1
//                2. 优化为io和计算分离，2线程并发做？
LogCleanWal::LoadSegmentResult LogCleanWal::load_segment(const std::string &filePath, uint64_t segId, const WalEntryLoadedCallback &callback) {
    auto fd = utils::FileUtils::Open(filePath, O_RDONLY, 0, 0);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " open segment file " << filePath << " error" << errmsg;
        return LoadSegmentResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    auto fileSize = utils::FileUtils::GetFileSize(fd);
    if (-1 == fileSize) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " get file size for " << filePath << " error " << errmsg;
        return LoadSegmentResult(Code::FileCorrupt, errmsg);
    }

    // check if file header is corrupt.
    if (fileSize < LOGCLEAN_WAL_MAGIC_NO_LEN) {
        LCLOGEFUN << " file " << filePath << " header is corrupt!";
        return LoadSegmentResult(Code::FileCorrupt, FileCorruptError);
    }

    auto headerMpo = common::g_pMemPool->Get(LOGCLEAN_WAL_MAGIC_NO_LEN);
    auto headerBuffer = (uchar*)(headerMpo->Pointer());
    auto nRead = utils::IOUtils::ReadFully_V4(fd, (char*)headerBuffer, LOGCLEAN_WAL_MAGIC_NO_LEN);
    if (-1 == nRead) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " read segment " << filePath << " error " << errmsg;
        return LoadSegmentResult(Code::FileSystemError, errmsg);
    }

    uint32_t magicNo = ByteOrderUtils::ReadUInt32(headerBuffer);
    if (LOGCLEAN_WAL_MAGIC_NO != magicNo) {
        LCLOGEFUN << filePath << " header is corrupt!";
        return LoadSegmentResult(Code::FileCorrupt, FileCorruptError);
    }
    headerMpo->Put();

    // load segment content
    auto segmentEntryOffset = uint32_t(LOGCLEAN_WAL_MAGIC_NO_LEN);
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
            LOGEFUN << " read segment " << filePath << " error " << errmsg;
            return LoadSegmentResult(Code::FileSystemError, errmsg);
        }

        if (0 == nRead) {
            mpo->Put();
            if (0 != lastLeftSize) {
                LCLOGEFUN << " segment file " << filePath << "corrupt";
                return LoadSegmentResult(Code::FileCorrupt, FileCorruptError);
            }
            return LoadSegmentResult(uint32_t(fileSize));
        }

        std::vector<WalEntry> entries;
        ok = (nRead == m_batchReadSize);
        auto availableBufferSize = nRead + lastLeftSize;
        lastLeftSize = 0; // 遗留的buffer + 新读取的buffer开始处理了，上一次遗留的size就无用了。
        uint32_t offset = 0;
        while (offset < availableBufferSize) {
            auto leftSize = availableBufferSize - offset;
            uint32_t len = 0;
            uint32_t walEntryLen = 0;
            bool notEnoughOneEntry = false;
            if (leftSize < LOGCLEAN_WAL_ENTRY_HEADER_LEN) {
                notEnoughOneEntry = true;
            } else {
                len = ByteOrderUtils::ReadUInt32(buffer + offset);
                walEntryLen = LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE + len;
                // 剩下的不够一个entry了
                if (leftSize < walEntryLen) {
                    notEnoughOneEntry = true;
                }
            }

            if (notEnoughOneEntry) { // 剩下的不够一个entry了
                if (!ok) { // 文件读尽了
                    mpo->Put();
                    LCLOGEFUN << " parse segment entry in " << filePath << " failed at offset " << offset;
                    return LoadSegmentResult(Code::FileCorrupt, FileCorruptError);
                }
                lastBuffer = buffer;
                pLastMpo = mpo;
                lastLeftSize = uint32_t(leftSize);
                lastOffset = offset;
                break;
            }

            auto startPosOffset = LOGCLEAN_WAL_ENTRY_HEADER_LEN + len;
            auto startPos = ByteOrderUtils::ReadUInt32(buffer + offset + startPosOffset);
            if (startPos != segmentEntryOffset) {
                mpo->Put();
                LCLOGEFUN << " parse segment entry in " << filePath << " failed at offset " << offset;
                return LoadSegmentResult(Code::FileCorrupt, FileCorruptError);
            }

            // auto version = *(buffer + offset + LOGCLEAN_WAL_SIZE_LEN);
            auto entryId = ByteOrderUtils::ReadUInt64(buffer + offset + LOGCLEAN_WAL_ENTRY_ID_OFFSET);
            auto contentStartPtr = buffer + offset + LOGCLEAN_WAL_CONTENT_OFFSET;
            auto contentEndPtr = contentStartPtr + len - 1;
            common::Buffer b;
            b.Refresh(contentStartPtr, contentEndPtr, contentStartPtr, contentEndPtr, nullptr, false);
            auto entry = m_entryCreator();
            if (!entry->Decode(b)) {
                mpo->Put();
                LCLOGEFUN << " decode entry at offset " << offset << " failed!";
                return LoadSegmentResult(Code::DecodeEntryError, DecodeEntryError);
            }

            auto we = WalEntry(entryId, entry);
            entries.push_back(std::move(we));
            auto forwardOffset = len + LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;
            offset += forwardOffset;
            segmentEntryOffset += forwardOffset;
            m_mpEntriesIdSegId[entryId] = segId;
            auto lastEntryId = entryId - 1;
            if (m_lastTruncMaxIdx > lastEntryId) {
                m_lastTruncMaxIdx = lastEntryId;
            }
            m_currentEntryIdx = entryId;
        }

        if (!lastBuffer) {
            mpo->Put();
        }
        callback(entries);
    }

    return LoadSegmentResult(uint32_t(fileSize));
}

LogCleanWal::ListSegmentsResult LogCleanWal::list_segments_asc() {
    std::vector<std::string> children;
    auto rc = utils::FileUtils::CollectFileChildren(m_sRootDir, [&](const std::string &p) {
        return p.find(LOGCLEAN_WAL_SEGMENT_PREFIX_NAME) != std::string::npos;
    }, children);

    if (0 != rc) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " list dir " << m_sRootDir << "log files error " << errmsg;
        return ListSegmentsResult(Code::FileSystemError, errmsg);
    }

    std::vector<LogCleanWalSegmentFileInfo> rs;
    for (auto &c : children) {
        auto idPos = c.rfind('.') + 1;
        auto idStr = c.substr(idPos);
        uint64_t id;
        auto ok = utils::CommonUtils::ToUint46_t(idStr, id);
        if (!ok) {
            LCLOGEFUN << " parse segment file " << c << " id error.";
            return ListSegmentsResult(Code::FileMameCorrupt, FileNameCorruptError);
        }
        LogCleanWalSegmentFileInfo f = {
            .Id = id,
            .Path = c
        };
        rs.push_back(f);
    }

    std::sort(rs.begin(), rs.end(), [](const LogCleanWalSegmentFileInfo &a, const LogCleanWalSegmentFileInfo &b) -> bool {
        return a.Id < b.Id;
    });

    return ListSegmentsResult(std::move(rs));
}

uint64_t LogCleanWal::get_uncompleted_trunc_segments_max_id() {
    if (utils::FileUtils::Exist(m_sTruncOKFlagFilePath)) {
        return LOGCLEAN_WAL_INVALID_SEGMENT_ID;
    }

    if (!utils::FileUtils::Exist(m_sTruncInfoFilePath)) {
        return LOGCLEAN_WAL_INVALID_SEGMENT_ID;
    }

    auto idStr = utils::FileUtils::ReadAllString(m_sTruncInfoFilePath);
    if (idStr.empty()) {
        return LOGCLEAN_WAL_INVALID_SEGMENT_ID;
    }
    uint64_t entryId;
    auto rs = utils::CommonUtils::ToUint46_t(idStr, entryId);
    if (!rs) {
        return LOGCLEAN_WAL_INVALID_SEGMENT_ID;
    }

    return entryId;
}

std::string LogCleanWal::generate_segment_file_path(uint64_t id) {
    std::stringstream ss;
    ss << m_sRootDir;
    ss << '/';
    ss << LOGCLEAN_WAL_SEGMENT_PREFIX_NAME;
    ss << ".";
    ss << id;

    return ss.str();
}

WalResult LogCleanWal::clean_trunc_status() {
    if (utils::FileUtils::Exist(m_sTruncInfoFilePath)) {
        if (-1 == unlink(m_sTruncInfoFilePath.c_str())) {
            auto errmsg = strerror(errno);
            LCLOGEFUN << " unlink " << m_sTruncInfoFilePath << "error " << errmsg;
            return WalResult(Code::FileSystemError, errmsg);
        }
    }

    if (utils::FileUtils::Exist(m_sTruncOKFlagFilePath)) {
        if (-1 == unlink(m_sTruncOKFlagFilePath.c_str())) {
            auto errmsg = strerror(errno);
            LCLOGEFUN << " unlink " << m_sTruncOKFlagFilePath << " error " << errmsg;
            return WalResult(Code::FileSystemError, errmsg);
        }
    }

    return WalResult(Code::OK);
}

WalResult LogCleanWal::create_new_segment_file() {
    if (-1 != m_currentSegFd) {
        if (-1 == close(m_currentSegFd)) {
            auto errmsg = strerror(errno);
            LOGEFUN << "close seg file " << m_currentSegmentId << " failed " << errmsg;
            return WalResult(Code::FileSystemError, errmsg);
        }
    }

    ++m_currentSegmentId;
    auto fp = generate_segment_file_path(m_currentSegmentId);
    m_currentSegFd = utils::FileUtils::Open(fp, O_WRONLY, O_CREAT, 0644);
    m_currentSegFilePath = std::move(fp);
    if (-1 == m_currentSegFd) {
        auto errmsg = strerror(errno);
        LOGEFUN << "create segment file " << m_currentSegFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    // write file header.
    uchar header[LOGCLEAN_WAL_MAGIC_NO_LEN];
    ByteOrderUtils::WriteUInt32(header, LOGCLEAN_WAL_MAGIC_NO);
    if (-1 == utils::IOUtils::WriteFully(m_currentSegFd, (char*)header, LOGCLEAN_WAL_MAGIC_NO_LEN)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " write file " << m_currentSegFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    m_curSegFileSize = LOGCLEAN_WAL_MAGIC_NO_LEN;
    return WalResult(Code::OK);
}

WalResult LogCleanWal::create_trunc_info(uint64_t segId) {
    auto fd = utils::FileUtils::Open(m_sTruncInfoFilePath, O_WRONLY, O_CREAT|O_TRUNC, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " open file " << m_sTruncInfoFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    auto segIdStr = utils::CommonUtils::ToString(segId);
    if (-1 == utils::IOUtils::WriteFully(fd, segIdStr.c_str(), segIdStr.size())) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " write truncate info file " << m_sTruncInfoFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    return WalResult(Code::OK);
}

WalResult LogCleanWal::create_trunc_ok_flag() {
    auto fd = utils::FileUtils::Open(m_sTruncOKFlagFilePath, O_WRONLY, O_CREAT, 0644);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " open file " << m_sTruncOKFlagFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }
    if (-1 == close(fd)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " close file " << m_sTruncOKFlagFilePath << " error " << errmsg;
        return WalResult(Code::FileSystemError, errmsg);
    }

    return WalResult(Code::OK);
}

// TODO(sunchao): 抽一个load entry？
LogCleanWal::LoadSegmentMaxEntryIdResult LogCleanWal::load_segment_max_entry_id(const std::string &fp) {
    auto fd = utils::FileUtils::Open(fp, O_RDONLY, 0, 0);
    if (-1 == fd) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " open segment " << fp << " error " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }

    common::FileCloser fc(fd);
    auto fileSize = utils::FileUtils::GetFileSize(fd);
    if (-1 == lseek((fd), fileSize - LOGCLEAN_WAL_START_POS_LEN, SEEK_SET)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " lseek file " << fp << " failed with errmsg " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }
    char buf4Len[LOGCLEAN_WAL_START_POS_LEN];
    auto n = utils::IOUtils::ReadFully_V4(fd, buf4Len, LOGCLEAN_WAL_START_POS_LEN);
    if (-1 == n) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " read segment " << fp << " error " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }

    uint32_t startPos = ByteOrderUtils::ReadUInt32((uchar*)buf4Len);
    if (-1 == lseek((fd), startPos, SEEK_SET)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " lseek file " << fp << " failed with errmsg " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }
    auto mpo = common::g_pMemPool->Get(LOGCLEAN_WAL_ENTRY_HEADER_LEN);
    auto buffer = (uchar*)mpo->Pointer();
    n = utils::IOUtils::ReadFully_V4(fd, (char*)buffer, LOGCLEAN_WAL_ENTRY_HEADER_LEN);
    if (-1 == n) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " read segment " << fp << " error " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }

    auto contentSize = ByteOrderUtils::ReadUInt32(buffer);
    //auto version = *(buffer + LOGCLEAN_WAL_SIZE_LEN);
    auto entryId = ByteOrderUtils::ReadUInt32(buffer + LOGCLEAN_WAL_ENTRY_ID_OFFSET);
    if (-1 == lseek((fd), contentSize, SEEK_CUR)) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " lseek file " << fp << " failed with errmsg " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }
    n = utils::IOUtils::ReadFully_V4(fd, buf4Len, LOGCLEAN_WAL_START_POS_LEN);
    if (-1 == n) {
        auto errmsg = strerror(errno);
        LCLOGEFUN << " read segment " << fp << " error " << errmsg;
        return LoadSegmentMaxEntryIdResult(Code::FileSystemError, errmsg);
    }

    auto checkStartPos = ByteOrderUtils::ReadUInt32((uchar*)buf4Len);
    if (startPos == checkStartPos) {
        return LoadSegmentMaxEntryIdResult(entryId);
    }

    // entry被破坏，直接返回SMLOG_INVALID_ENTRY_ID.
    return LoadSegmentMaxEntryIdResult(Code::FileCorrupt, FileCorruptError);
}
}
}
