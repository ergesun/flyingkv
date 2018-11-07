/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <string>
#include <gtest/gtest.h>

#include "../../wal/wal-factory.h"
#include "../../wal/wals/log-clean-wal.h"

#include "entry/test-entry.h"
#include "entry-handler/test-entry-handler.h"
#include "../../utils/file-utils.h"
#include "../../utils/io-utils.h"
#include "../../common/global-vars.h"
#include "../../utils/codec-utils.h"

using namespace flyingkv;
using namespace flyingkv::wal;
using namespace flyingkv::waltest;
using namespace flyingkv::common;

#define TEST_ENTRY_VERSION               1
const uint32_t DEFAULT_SEGMENTS_SIZE   = 1024 * 1024 * 100;
const uint32_t DEFAULT_BATCH_READ_SIZE = 1024 * 1024 * 10;

#define INVALID_WAL_ROOT  "/invalid_wal_root"
#define INVALID_WAL_TYPE  "invalid_wal_type"
#define WAL_TYPE          "log-clean"
#define WAL_ROOT          "/tmp/wal-test"

// TODO(sunchao): mock fs以便覆盖文件系统错误以便更全面的分支覆盖？感觉没必要。
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    umask(0);
    common::initialize();
    return RUN_ALL_TESTS();
}

TEST(LCWALTest, ArgsTest) {
    auto handler = new TestEntryHandler();
    auto pWalConf = new wal::LogCleanWalConfig(INVALID_WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler),
                                               TEST_ENTRY_VERSION, DEFAULT_SEGMENTS_SIZE, DEFAULT_BATCH_READ_SIZE);
    IWal *pWal = WALFactory::CreateInstance(pWalConf);
    EXPECT_EQ(pWal, nullptr);
    delete pWalConf;
    pWalConf = new wal::LogCleanWalConfig(WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler),
                                               TEST_ENTRY_VERSION, DEFAULT_SEGMENTS_SIZE, DEFAULT_BATCH_READ_SIZE);
    pWal = WALFactory::CreateInstance(pWalConf);
    EXPECT_NE(pWal, nullptr);
    delete pWalConf;
}

TEST(LCWALTest, InitTest) {
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    auto handler = new TestEntryHandler();

    // - invalid root
    auto pWalConf = new wal::LogCleanWalConfig(WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler),
                                               TEST_ENTRY_VERSION, DEFAULT_SEGMENTS_SIZE, DEFAULT_BATCH_READ_SIZE);
    IWal *pWal = WALFactory::CreateInstance(pWalConf);
    auto rs = pWal->Init();
    EXPECT_EQ(rs.Rc, Code::FileSystemError);
    delete pWalConf;

    pWalConf = new wal::LogCleanWalConfig(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler),
                                               TEST_ENTRY_VERSION, DEFAULT_SEGMENTS_SIZE, DEFAULT_BATCH_READ_SIZE);
    pWal = WALFactory::CreateInstance(pWalConf);
    auto plcWal = dynamic_cast<LogCleanWal*>(pWal);
    rs = plcWal->Init();
    EXPECT_EQ(Code::OK, rs.Rc);

    plcWal->create_trunc_ok_flag();

    // has ok flag
    auto id = plcWal->get_uncompleted_trunc_segments_max_id();
    EXPECT_EQ(LOGCLEAN_WAL_INVALID_SEGMENT_ID, id);

    // no trunc info
    plcWal->clean_trunc_status();
    id = plcWal->get_uncompleted_trunc_segments_max_id();
    EXPECT_EQ(LOGCLEAN_WAL_INVALID_SEGMENT_ID, id);

    // has trunc info
    plcWal->create_trunc_info(6);
    id = plcWal->get_uncompleted_trunc_segments_max_id();
    EXPECT_EQ(6, id);

    // corrupt trunc info
    auto fd = utils::FileUtils::Open(plcWal->m_sTruncInfoFilePath, O_WRONLY, 0, 0);
    utils::IOUtils::WriteFully(fd, "invalid number", 14);
    close(fd);
    id = plcWal->get_uncompleted_trunc_segments_max_id();
    EXPECT_EQ(LOGCLEAN_WAL_INVALID_SEGMENT_ID, id);

    // generate segment file path test
    auto seg1Path = std::string(WAL_ROOT) + "/" + LOGCLEAN_WAL_SEGMENT_PREFIX_NAME + ".1";
    auto seg1GenPath = plcWal->generate_segment_file_path(1);
    EXPECT_EQ(seg1Path, seg1GenPath);

    delete pWal;
    // list segments test
    pWal = WALFactory::CreateInstance(pWalConf);
    plcWal = dynamic_cast<LogCleanWal*>(pWal);
    plcWal->Init();
    rs = plcWal->create_new_segment_file();
    EXPECT_EQ(Code::OK, rs.Rc);

    auto lsSegRs = plcWal->list_segments_asc();
    EXPECT_EQ(2, lsSegRs.Segments.size());
    EXPECT_EQ(1, lsSegRs.Segments[0].Id);
    EXPECT_EQ(seg1Path, lsSegRs.Segments[0].Path);
    EXPECT_EQ(2, lsSegRs.Segments[1].Id);

    rename(seg1Path.c_str(), (seg1Path + "x").c_str());
    lsSegRs = plcWal->list_segments_asc();
    EXPECT_EQ(lsSegRs.Rc, Code::FileMameCorrupt);

    rs = plcWal->Init();
    EXPECT_EQ(rs.Rc, Code::FileMameCorrupt);

    // 从0初始化成功
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    rs = plcWal->Init();
    EXPECT_EQ(Code::OK, rs.Rc);
    EXPECT_EQ(plcWal->m_minSegmentId, 1);
    EXPECT_EQ(plcWal->m_currentSegmentId, 1);
    EXPECT_NE(plcWal->m_currentSegFd, -1);

    // 从有segments初始化
    plcWal->create_new_segment_file();

    delete pWal;
    pWal = WALFactory::CreateInstance(pWalConf);
    plcWal = dynamic_cast<LogCleanWal*>(pWal);
    rs = plcWal->Init();
    EXPECT_EQ(rs.Rc, Code::OK);
    EXPECT_EQ(plcWal->m_minSegmentId, 1);
    EXPECT_EQ(plcWal->m_currentSegmentId, 2);
    EXPECT_NE(plcWal->m_currentSegFd, -1);

    delete pWalConf;
    delete pWal;
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
}

TEST(LCWALTest, AppendEntryTest) {
    auto handler = new TestEntryHandler();
    auto loadHandler = std::bind(&TestEntryHandler::OnLoad, handler, std::placeholders::_1);
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    auto pWalConf = new wal::LogCleanWalConfig(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler),
                                               TEST_ENTRY_VERSION, DEFAULT_SEGMENTS_SIZE, DEFAULT_BATCH_READ_SIZE);
    auto pWal = dynamic_cast<LogCleanWal*>(WALFactory::CreateInstance(pWalConf));
    delete pWalConf;
    auto rs = pWal->AppendEntry(nullptr);
    EXPECT_EQ(rs.Rc, Code::Uninited);

    pWal->Init();
    rs = pWal->AppendEntry(nullptr);
    EXPECT_EQ(rs.Rc, Code::Unloaded);
    pWal->Load(loadHandler);

    // size overflow
    auto testEntry = dynamic_cast<TestEntry*>(handler->CreateNewEntryWithContent("overflow"));
    testEntry->SetIllusorySize(pWal->m_maxEntrySize + 1);
    rs = pWal->AppendEntry(testEntry);
    EXPECT_EQ(rs.Rc, Code::EntryBytesSizeOverflow);
    delete testEntry;

    // cannot encoded
    auto entry = handler->CreateNewEntry();
    testEntry = dynamic_cast<TestEntry*>(entry);
    testEntry->SetCanEncode(false);
    rs = pWal->AppendEntry(testEntry);
    EXPECT_EQ(rs.Rc, Code::EncodeEntryError);
    delete entry;

    // no content entry
    entry = handler->CreateNewEntry();
    rs = pWal->AppendEntry(entry);
    EXPECT_EQ(rs.Rc, Code::OK);
    delete entry;

    // has content entry
    entry = handler->CreateNewEntryWithContent("abc");
    rs = pWal->AppendEntry(entry);
    EXPECT_EQ(rs.Rc, Code::OK);
    delete entry;

    entry = handler->CreateNewEntryWithContent("defgh");
    rs = pWal->AppendEntry(entry);
    EXPECT_EQ(rs.Rc, Code::OK);

    // check contents
    auto segmentFp = pWal->m_currentSegFilePath;
    auto fd = utils::FileUtils::Open(segmentFp, O_RDONLY, 0, 0);
    auto fileSize = utils::FileUtils::GetFileSize(fd);
    auto expectFileSize = LOGCLEAN_WAL_MAGIC_NO_LEN + LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE * 3 + 3 + 5;
    uint32_t offset = 0;
    /// 1. check file size
    EXPECT_EQ(fileSize, expectFileSize);
    auto buf = new uchar[fileSize];
    utils::IOUtils::ReadFully_V4(fd, (char*)buf, size_t(fileSize));
    /// 2. check file header
    auto magicNo = ByteOrderUtils::ReadUInt32(buf);
    EXPECT_EQ(magicNo, LOGCLEAN_WAL_MAGIC_NO);
    offset += LOGCLEAN_WAL_MAGIC_NO_LEN;
    /// 3. check entries
    //// 3.1 1st empty entry
    auto entryLen = ByteOrderUtils::ReadUInt32(buf + offset);
    EXPECT_EQ(entryLen, 0);
    auto startPos = ByteOrderUtils::ReadUInt32(buf + offset + (LOGCLEAN_WAL_ENTRY_HEADER_LEN));
    EXPECT_EQ(startPos, offset);
    offset += LOGCLEAN_WAL_SIZE_LEN;
    auto version = *(buf + offset);
    EXPECT_EQ(version, 1);
    offset += LOGCLEAN_WAL_VERSION_LEN;
    auto entryId = ByteOrderUtils::ReadUInt64(buf + offset);
    EXPECT_EQ(entryId, 1);
    offset += LOGCLEAN_WAL_ENTRY_ID_LEN + LOGCLEAN_WAL_START_POS_LEN;

    //// 3.2 second entry
    entryLen = ByteOrderUtils::ReadUInt32(buf + offset);
    EXPECT_EQ(entryLen, 3);
    startPos = ByteOrderUtils::ReadUInt32(buf + offset + entryLen + LOGCLEAN_WAL_ENTRY_HEADER_LEN);
    EXPECT_EQ(startPos, offset);
    offset += LOGCLEAN_WAL_SIZE_LEN;
    version = *(buf + offset);
    EXPECT_EQ(version, 1);
    offset += LOGCLEAN_WAL_VERSION_LEN;
    entryId = ByteOrderUtils::ReadUInt64(buf + offset);
    EXPECT_EQ(entryId, 2);
    offset += LOGCLEAN_WAL_ENTRY_ID_LEN;
    auto strtmp = new char[4];
    memcpy(strtmp, buf + offset, 3);
    strtmp[3] = 0;
    EXPECT_STREQ(strtmp, "abc");
    DELETE_ARR_PTR(strtmp);
    offset += 3 + LOGCLEAN_WAL_START_POS_LEN;

    //// 3.3 third entry
    entryLen = ByteOrderUtils::ReadUInt32(buf + offset);
    EXPECT_EQ(entryLen, 5);
    startPos = ByteOrderUtils::ReadUInt32(buf + offset + entryLen + LOGCLEAN_WAL_ENTRY_HEADER_LEN);
    EXPECT_EQ(startPos, offset);
    offset += LOGCLEAN_WAL_SIZE_LEN;
    version = *(buf + offset);
    EXPECT_EQ(version, 1);
    offset += LOGCLEAN_WAL_VERSION_LEN;
    entryId = ByteOrderUtils::ReadUInt64(buf + offset);
    EXPECT_EQ(entryId, 3);
    offset += LOGCLEAN_WAL_ENTRY_ID_LEN;
    strtmp = new char[6];
    memcpy(strtmp, buf + offset, 5);
    strtmp[5] = 0;
    EXPECT_STREQ(strtmp, "defgh");

    DELETE_ARR_PTR(buf);
    close(fd);
}

TEST(LCWALTest, LoadTest) {
    auto handler = new TestEntryHandler();
    auto loadHandler = std::bind(&TestEntryHandler::OnLoad, handler, std::placeholders::_1);
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    auto pWalConf = new wal::LogCleanWalConfig(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler),
                                               TEST_ENTRY_VERSION, DEFAULT_SEGMENTS_SIZE, DEFAULT_BATCH_READ_SIZE);
    auto pWal = dynamic_cast<LogCleanWal*>(WALFactory::CreateInstance(pWalConf));
    // uninitialized
    auto rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::Uninited);
    // 1. no segments and entries
    pWal->Init();
    rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::OK);
    EXPECT_EQ(handler->m_vLoadEntries.empty(), true);

    // 2. has entries
    /// 2.1 insert some entries
    auto entry = handler->CreateNewEntry();
    auto ars = pWal->AppendEntry(entry);
    EXPECT_EQ(ars.Rc, Code::OK);
    delete entry;

    entry = handler->CreateNewEntryWithContent("abc");
    ars = pWal->AppendEntry(entry);
    EXPECT_EQ(ars.Rc, Code::OK);
    delete entry;

    entry = handler->CreateNewEntryWithContent("defgh");
    ars = pWal->AppendEntry(entry);
    EXPECT_EQ(ars.Rc, Code::OK);

    delete pWal;
    /// 2.2 load
    pWal = dynamic_cast<LogCleanWal*>(WALFactory::CreateInstance(pWalConf));
    pWal->Init();
    rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::OK);
    EXPECT_EQ(handler->m_vLoadEntries.size(), 3);

    // 2.3 small batch
    pWalConf->ReadBatchSize = LOGCLEAN_WAL_ENTRY_EXTRA_FIELDS_SIZE + 5;
    auto pWal2 = dynamic_cast<LogCleanWal*>(WALFactory::CreateInstance(pWalConf));
    pWal2->Init();
    rs = pWal2->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::OK);
    delete pWal2;

    // 3. destroy last entry start offset
    auto fd = utils::FileUtils::Open(pWal->m_vInitSegments[0].Path, O_WRONLY, 0, 0);
    auto fileSize  = utils::FileUtils::GetFileSize(fd);
    lseek(fd, 1, SEEK_END);
    utils::IOUtils::WriteFully(fd, "abcdfsfsdfdsfdsfdsfsdafdsafdafsdfsdfdfdfsdfsdfddddsfsdafdsfdsfdsfsadfsdfdsfdsfsdfsdf", 3);
    rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::FileCorrupt);

    ftruncate(fd, fileSize - 5);
    rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::FileCorrupt);
    delete pWal;

    delete pWalConf;
}

TEST(LCWALTest, TruncateTest) {

}