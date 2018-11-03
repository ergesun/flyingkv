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
LogCleanWal::LogCleanWal(const std::string &rootDir, common::EntryCreateHandler &&entryCreateHandler) :
        m_sRootDir(rootDir), m_entryCreator(std::move(entryCreateHandler)) {
    if (rootDir.empty()) {
        LOGFFUN << "wal root dir is empty";
    }

    m_sRootDir = rootDir;
    m_sTruncInfoFilePath = rootDir + "/" + LOGCLEAN_WAL_TRUNC_META_NAME;
    m_sTruncOKFlagFilePath = rootDir + "/" + LOGCLEAN_WAL_TRUNC_OK_NAME;
    if (-1 == utils::FileUtils::CreateDirPath(m_sRootDir, 0775)) {
        LOGFFUN << "create wal dir " << m_sRootDir.c_str() << " failed.";
    }

    m_vInitSegments = init();
}

LogCleanWal::~LogCleanWal() {
    if (-1 != m_currentSegFd) {
        close(m_currentSegFd);
        m_currentSegFd = -1;
    }
}

// TODO(sunchao): 抽象一个编码函数，对应不同的log version。目前这个只能编码version=1
uint64_t LogCleanWal::AppendEntry(common::IEntry *entry) {
    if (UNLIKELY(!m_bLoaded)) {
        LOGFFUN << "simple wal has not been loaded.";
    }
    std::shared_ptr<common::Buffer> eb;
    if (UNLIKELY(!entry->Encode(eb))) {
        LOGFFUN << "simple wal encode entry error";
    }

    auto walEntryStartOffset = m_curSegFileSize;
    auto rawEntrySize = uint32_t(eb->AvailableLength());
    auto walEntrySize = rawEntrySize + LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE;
    if (walEntrySize > LOGCLEAN_WAL_ENTRY_MAX_SIZE) {
        LOGFFUN << "entry size cannot be larger than " << LOGCLEAN_WAL_ENTRY_MAX_SIZE << " bytes";
    }

    // mpo will be Putted in Buffer 'b'.
    auto mpo = common::g_pMemPool->Get(uint32_t(walEntrySize));
    auto bufferStart = (uchar*)(mpo->Pointer());
    common::Buffer wb;
    wb.Refresh(bufferStart, bufferStart + walEntrySize - 1, bufferStart, bufferStart + walEntrySize - 1, mpo);
    // size
    ByteOrderUtils::WriteUInt32(wb.GetPos(), uint32_t(rawEntrySize));
    wb.MoveHeadBack(LOGCLEAN_WAL_SIZE_LEN);

    // version
    *(wb.GetPos()) = LOGCLEAN_WAL_VERSION;
    wb.MoveHeadBack(LOGCLEAN_WAL_SIZE_LEN);

    // log id
    ByteOrderUtils::WriteUInt64(wb.GetPos(), uint32_t(m_currentEntryIdx));
    wb.MoveHeadBack(LOGCLEAN_WAL_ENTRY_ID_LEN);

    // content
    memcpy(eb->GetPos(), wb.GetPos(), size_t(rawEntrySize));
    wb.MoveHeadBack(uint32_t(rawEntrySize));

    // offset
    ByteOrderUtils::WriteUInt32(wb.GetPos(), walEntryStartOffset);

    WriteFileFullyWithFatalLOG(m_currentSegFd, (char*)(wb.GetStart()), size_t(walEntrySize), m_currentSegFilePath);
    FDataSyncFileWithFatalLOG(m_currentSegFd, m_currentSegFilePath);
    m_curSegFileSize += walEntrySize;
    m_mpEntriesIdSegId[m_currentEntryIdx] = m_currentSegmentId;
    ++m_currentEntryIdx;
    if (m_curSegFileSize >= LOGCLEAN_WAL_SEGMENT_MAX_SIZE) {
        create_new_segment_file();
    }

    return m_currentEntryIdx;
}

void LogCleanWal::Load(const WalEntryLoadedCallback &callback) {
    if (m_vInitSegments.empty()) {
        return;
    }

    m_minSegmentId = m_vInitSegments[0].Id;
    auto maxIdx = m_vInitSegments.size() - 1;
    for (size_t i = 0; i <= maxIdx; ++i) {
        auto segSize = load_segment(m_vInitSegments[i].Path, m_vInitSegments[i].Id, callback);
        if (i == maxIdx) {
            m_curSegFileSize = segSize;
        }
    }
}

bool LogCleanWal::TruncateAhead(uint64_t id) {
    if (UNLIKELY(!m_bLoaded)) {
        LOGFFUN << "segment wal has not been loaded.";
    }

    auto iter = m_mpEntriesIdSegId.find(id);
    if (m_mpEntriesIdSegId.end() == iter) {
        LOGEFUN << "cannot find truncate entry id " << id;
        return false;
    }

    if (0 == m_curSegFileSize) {
        LOGIFUN << "no entry, so no need to truncate.";
        return true;
    }

    // 1. 写入truncate信息
    write_trunc_info(id);

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
        LOGFFUN << "access file " << fp << " error " << strerror(errno);
    }

    if (0 == nextSegSize) {
        --maxTruncateSegId;
    }

    uint64_t maxCleanEntryId = LOGCLEAN_WAL_INVALID_ENTRY_ID;
    for (uint64_t i = m_minSegmentId; i <= maxTruncateSegId; ++i) {
        auto fp = generate_segment_file_path(i);
        if (i == maxTruncateSegId) {
            maxCleanEntryId = load_segment_max_entry_id(fp);
        }
        if (-1 == unlink(fp.c_str())) {
            LOGFFUN << "unlink segment " << fp << " error " << strerror(errno);
        }
    }

    if (LOGCLEAN_WAL_INVALID_ENTRY_ID == maxCleanEntryId) {
        m_minSegmentId = LOGCLEAN_WAL_MIN_ENTRY_ID;
        return true;
    }

    m_minSegmentId = maxTruncateSegId + 1;
    for (auto i = m_minSegmentId; i <= maxCleanEntryId; ++i) {
        auto iter = m_mpEntriesIdSegId.find(i);
        if (m_mpEntriesIdSegId.end() != iter) {
            m_mpEntriesIdSegId.erase(iter);
        }
    }

    write_trunc_ok_flag();
    clean_trunc_status();
    return true;
}

/**
 * 清理未完成的trunc，初始化segment相关信息
 */
std::vector<LogCleanWalSegmentFileInfo> LogCleanWal::init() {
    auto maxTruncUncompletedSegId = get_uncompleted_trunc_segments_max_id();
    auto segments = list_segments_asc();
    if (LOGCLEAN_WAL_INVALID_SEGMENT_ID != maxTruncUncompletedSegId) {
        if (segments.empty()) { // 没有segment了,只是trunc info没来得及清理就挂了
            clean_trunc_status();
            return segments;
        }

        auto minSegmentId = segments[0].Id;
        for (uint64_t i = minSegmentId; i <= maxTruncUncompletedSegId; ++i) {
            auto cleanSegFilePath = generate_segment_file_path(i);
            if (-1 != unlink(cleanSegFilePath.c_str())) {
                LOGFFUN << "clean segment " << cleanSegFilePath << " error " << strerror(errno);
            }
        }

        segments.erase(std::remove_if(segments.begin(), segments.end(),
                                      [maxTruncUncompletedSegId](const LogCleanWalSegmentFileInfo &s) {
                                            return s.Id <= maxTruncUncompletedSegId;
                                        }
                                     ),
                       segments.end());
    }

        clean_trunc_status();
    if (segments.empty()) {
        m_minSegmentId = 1; // 初始值
        m_currentSegmentId = 1;
    } else {
        m_minSegmentId = segments[0].Id;
        m_currentSegmentId = segments[segments.size() - 1].Id;
    }

    auto curSegmentFilePath = generate_segment_file_path(m_currentSegmentId);
    auto fd = utils::FileUtils::Open(curSegmentFilePath, O_RDONLY, O_CREAT, 0644);
    if (-1 == fd) {
        LOGFFUN << "open sm log file " << curSegmentFilePath << " failed with errmsg " << strerror(errno);
    }

    m_currentSegFd = fd;
    m_currentSegFilePath = std::move(curSegmentFilePath);
    LSeekFileWithFatalLOG(fd, utils::FileUtils::GetFileSize(fd), SEEK_SET, m_currentSegFilePath);

    return segments;
}

// TODO(sunchao): 抽象一个解析函数，对应不同的log version。目前这个只能解析version=1
uint32_t LogCleanWal::load_segment(const std::string &filePath, uint64_t segId, const WalEntryLoadedCallback &callback) {
    auto fd = utils::FileUtils::Open(filePath, O_RDONLY, 0, 0);
    if (-1 == fd) {
        LOGFFUN << "open sm log file " << filePath << " failed with errmsg " << strerror(errno);
    }

    common::FileCloser fc(fd);
    // 1. check if file is empty.
    auto fileSize = utils::FileUtils::GetFileSize(fd);
    if (-1 == fileSize) {
        LOGFFUN << "get file size for " << filePath << " failed with errmsg " << strerror(errno);
    }
    if (0 == fileSize) {
        LOGDFUN3("sm log file ", filePath, " is empty!");
        return uint32_t(fileSize);
    }

    // 2. check if file header is corrupt.
    if (fileSize < LOGCLEAN_WAL_MAGIC_NO_LEN) {
        LOGFFUN << "sm log file " << filePath << " header is corrupt!";
    }

    auto headerMpo = common::g_pMemPool->Get(LOGCLEAN_WAL_MAGIC_NO_LEN);
    auto headerBuffer = (uchar*)(headerMpo->Pointer());
    auto nRead = utils::IOUtils::ReadFully_V4(fd, (char*)headerBuffer, LOGCLEAN_WAL_MAGIC_NO_LEN);
    if (-1 == nRead) {
        LOGFFUN << "read segment " << filePath << " error";
    }

    uint32_t magicNo = ByteOrderUtils::ReadUInt32(headerBuffer);
    if (LOGCLEAN_WAL_MAGIC_NO != magicNo) {
        LOGFFUN << "sm log " << filePath << " header is corrupt!";
    }
    headerMpo->Put();

    auto segmentEntryOffset = uint32_t(LOGCLEAN_WAL_MAGIC_NO_LEN);
    // 3. load segment content

    sys::MemPool::MemObject *pLastMpo = nullptr;
    uchar *lastBuffer = nullptr;
    uint32_t lastLeftSize = 0;
    bool ok = true;
    while (ok) {
        auto readSize = LOGCLEAN_WAL_READ_BATCH_SIZE + lastLeftSize;
        auto mpo = common::g_pMemPool->Get(readSize);
        auto buffer = (uchar*)(mpo->Pointer());
        if (lastBuffer) {
            auto lastOffset = LOGCLEAN_WAL_READ_BATCH_SIZE - lastLeftSize;
            memcpy(buffer, lastBuffer + lastOffset, lastLeftSize);
            lastBuffer = nullptr;
            pLastMpo->Put();
            lastLeftSize = 0;
        }

        nRead = utils::IOUtils::ReadFully_V4(fd, (char*)buffer + lastLeftSize, LOGCLEAN_WAL_READ_BATCH_SIZE);
        if (-1 == nRead) {
            LOGFFUN << "read segment " << filePath << " error";
        }

        if (0 == nRead) {
            if (0 != lastLeftSize) {
                LOGFFUN << "segment file " << filePath << "corrupt";
            }
            return fileSize;
        }

        std::vector<WalEntry> entries;
        ok = (nRead == LOGCLEAN_WAL_READ_BATCH_SIZE);
        auto availableBufferSize = nRead + lastLeftSize;
        uint32_t offset = 0;
        while (offset < availableBufferSize) {
            auto leftSize = availableBufferSize - offset;
            // 剩下的不够一个entry了
            if (leftSize < (LOGCLEAN_WAL_SIZE_LEN + LOGCLEAN_WAL_VERSION_LEN + LOGCLEAN_WAL_ENTRY_ID_LEN)) {
                lastBuffer = buffer;
                pLastMpo = mpo;
                lastLeftSize = uint32_t(leftSize);
                break;
            }

            auto len = ByteOrderUtils::ReadUInt32(buffer + offset);
            auto walEntryLen = LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE + len;
            // 剩下的不够一个entry了
            if (leftSize < walEntryLen) {
                lastBuffer = buffer;
                pLastMpo = mpo;
                lastLeftSize = uint32_t(leftSize);
                break;
            }

            auto startPosOffset = LOGCLEAN_WAL_SIZE_LEN + LOGCLEAN_WAL_VERSION_LEN + LOGCLEAN_WAL_ENTRY_ID_LEN + len;
            auto startPos = ByteOrderUtils::ReadUInt32(buffer + offset + startPosOffset);
            if (startPos != segmentEntryOffset) {
                LOGFFUN << "parse sm segment entry in " << filePath << " failed at offset " << offset;
            }

            // auto version = *(buffer + offset + LOGCLEAN_WAL_SIZE_LEN);
            auto entryId = ByteOrderUtils::ReadUInt64(buffer + offset + LOGCLEAN_WAL_ENTRY_ID_OFFSET);
            auto contentStartPtr = buffer + offset + LOGCLEAN_WAL_CONTENT_OFFSET;
            auto contentEndPtr = buffer + len - 1;
            common::Buffer b;
            b.Refresh(contentStartPtr, contentEndPtr, contentStartPtr, contentEndPtr, nullptr);
            auto entry = m_entryCreator();
            if (!entry->Decode(b)) {
                LOGFFUN << "deserialize sm log entry at offset " << offset << " failed!";
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

    return fileSize;
}

std::vector<LogCleanWalSegmentFileInfo> LogCleanWal::list_segments_asc() {
    std::vector<std::string> children;
    auto rc = utils::FileUtils::CollectFileChildren(m_sRootDir, [&](const std::string &p) {
        return p.find(LOGCLEAN_WAL_SEGMENT_PREFIX_NAME) != std::string::npos;
    }, children);

    if (0 != rc) {
        LOGFFUN << "list dir " << m_sRootDir << "log files error " << strerror(rc);
    }

    std::vector<LogCleanWalSegmentFileInfo> rs;
    for (auto &c : children) {
        auto idPos = c.rfind('.') + 1;
        auto id = utils::CommonUtils::ToInteger<uint64_t>(c.substr(idPos));
        LogCleanWalSegmentFileInfo f = {
            .Id = id,
            .Path = c
        };
        rs.push_back(f);
    }

    std::sort(rs.begin(), rs.end(), [](const LogCleanWalSegmentFileInfo &a, const LogCleanWalSegmentFileInfo &b) -> bool {
        return a.Id < b.Id;
    });

    return rs;
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

    return utils::CommonUtils::ToInteger<uint64_t>(idStr);
}

std::string LogCleanWal::generate_segment_file_path(uint64_t id) {
    std::stringstream ss;
    ss << m_sRootDir;
    ss << '/';
    ss << LOGCLEAN_WAL_SEGMENT_PREFIX_NAME;
    ss << id;

    return ss.str();
}

void LogCleanWal::clean_trunc_status() {
    if (utils::FileUtils::Exist(m_sTruncInfoFilePath)) {
        if (-1 != unlink(m_sTruncInfoFilePath.c_str())) {
            LOGFFUN << "clean " << m_sTruncInfoFilePath << "error " << strerror(errno);
        }
    }

    if (utils::FileUtils::Exist(m_sTruncOKFlagFilePath)) {
        if (-1 != unlink(m_sTruncOKFlagFilePath.c_str())) {
            LOGFFUN << "clean " << m_sTruncOKFlagFilePath << "error " << strerror(errno);
        }
    }
}

void LogCleanWal::create_new_segment_file() {
    if (-1 != m_currentSegFd) {
        if (-1 == close(m_currentSegFd)) {
            LOGFFUN << "close seg file " << m_currentSegmentId << " failed " << strerror(errno);
        }
    }

    ++m_currentSegmentId;
    auto fp = generate_segment_file_path(m_currentSegmentId);
    m_currentSegFd = utils::FileUtils::Open(fp, O_WRONLY, O_CREAT, 644);
    m_currentSegFilePath = std::move(fp);
    if (-1 == m_currentSegFd) {
        LOGFFUN << "create segment file " << m_currentSegFilePath << " error " << strerror(errno);
    }

    // write file header.
    uchar header[LOGCLEAN_WAL_MAGIC_NO_LEN];
    ByteOrderUtils::WriteUInt32(header, LOGCLEAN_WAL_MAGIC_NO);
    WriteFileFullyWithFatalLOG(m_currentSegFd, (char*)header, LOGCLEAN_WAL_MAGIC_NO_LEN, m_currentSegFilePath);

    m_curSegFileSize = LOGCLEAN_WAL_MAGIC_NO_LEN;
}

void LogCleanWal::write_trunc_info(uint64_t segId) {
    auto fd = utils::FileUtils::Open(m_sTruncInfoFilePath, O_WRONLY, O_CREAT|O_TRUNC, 644);
    if (-1 == fd) {
        LOGFFUN << "open file " << m_sTruncInfoFilePath << " error " << strerror(errno);
    }

    common::FileCloser fc(fd);
    auto segIdStr = utils::CommonUtils::ToString(segId);
    if (-1 == utils::IOUtils::WriteFully(fd, segIdStr.c_str(), segIdStr.size())) {
        LOGFFUN << "write truncate info file " << m_sTruncInfoFilePath << " error " << strerror(errno);
    }
}

void LogCleanWal::write_trunc_ok_flag() {
    auto fd = utils::FileUtils::Open(m_sTruncOKFlagFilePath, O_WRONLY, O_CREAT, 644);
    if (-1 == fd) {
        LOGFFUN << "open file " << m_sTruncOKFlagFilePath << " error " << strerror(errno);
    }
    if (-1 == close(fd)) {
        LOGFFUN << "create file trunc ok flag file " << m_sTruncOKFlagFilePath << " error " << strerror(errno);
    }
}

// TODO(sunchao): 抽一个load entry？
uint64_t LogCleanWal::load_segment_max_entry_id(const std::string &fp) {
    auto fd = utils::FileUtils::Open(fp, O_RDONLY, 0, 0);
    if (-1 == fd) {
        LOGFFUN << "open segment " << fp << " error " << strerror(errno);
    }

    common::FileCloser fc(fd);
    LSeekFileWithFatalLOG(fd, 3, SEEK_END, fp);
    char buf4Len[LOGCLEAN_WAL_START_POS_LEN];
    auto n = utils::IOUtils::ReadFully_V4(fd, buf4Len, LOGCLEAN_WAL_START_POS_LEN);
    if (-1 == n) {
        LOGFFUN << "read segment " << fp << " error " << strerror(errno);
    }

    uint32_t startPos = ByteOrderUtils::ReadUInt32((uchar*)buf4Len);
    LSeekFileWithFatalLOG(fd, startPos, SEEK_SET, fp);
    auto mpo = common::g_pMemPool->Get(LOGCLEAN_WAL_ENTRY_HEADER_LEN);
    auto buffer = (uchar*)mpo->Pointer();
    n = utils::IOUtils::ReadFully_V4(fd, (char*)buffer, LOGCLEAN_WAL_ENTRY_HEADER_LEN);
    if (-1 == n) {
        LOGFFUN << "read segment " << fp << " error " << strerror(errno);
    }

    auto contentSize = ByteOrderUtils::ReadUInt32(buffer);
    //auto version = *(buffer + LOGCLEAN_WAL_SIZE_LEN);
    auto entryId = ByteOrderUtils::ReadUInt32(buffer + LOGCLEAN_WAL_ENTRY_ID_OFFSET);
    LSeekFileWithFatalLOG(fd, contentSize, SEEK_CUR, fp);
    n = utils::IOUtils::ReadFully_V4(fd, buf4Len, LOGCLEAN_WAL_START_POS_LEN);
    if (-1 == n) {
        LOGFFUN << "read segment " << fp << " error " << strerror(errno);
    }

    auto checkStartPos = ByteOrderUtils::ReadUInt32((uchar*)buf4Len);
    if (startPos == checkStartPos) {
        return entryId;
    }

    // entry被破坏，直接返回SMLOG_INVALID_ENTRY_ID.
    return LOGCLEAN_WAL_INVALID_ENTRY_ID;
}
}
}
