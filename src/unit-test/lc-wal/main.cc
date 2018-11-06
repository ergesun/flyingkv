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

using namespace flyingkv;
using namespace flyingkv::wal;
using namespace flyingkv::waltest;
using namespace flyingkv::common;

#define INVALID_WAL_ROOT  "/invalid_wal_root"
#define INVALID_WAL_TYPE  "invalid_wal_type"
#define WAL_TYPE  "log-clean"
#define WAL_ROOT  "/tmp/wal-test"

// TODO(sunchao): mock fs以便覆盖文件系统错误以便更全面的分支覆盖？感觉没必要。
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    umask(0);
    common::initialize();
    return RUN_ALL_TESTS();
}

TEST(LCWALTest, ArgsTest) {
    auto handler = new TestEntryHandler();
    IWal *pWal = WALFactory::CreateInstance(INVALID_WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
    EXPECT_EQ(pWal, nullptr);

    pWal = WALFactory::CreateInstance(WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
    EXPECT_NE(pWal, nullptr);
}

TEST(LCWALTest, InitTest) {
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    auto handler = new TestEntryHandler();

    // - invalid root
    IWal *pWal = WALFactory::CreateInstance(WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
    auto rs = pWal->Init();
    EXPECT_EQ(rs.Rc, Code::FileSystemError);

    pWal = WALFactory::CreateInstance(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
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
    pWal = WALFactory::CreateInstance(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
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
    pWal = WALFactory::CreateInstance(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
    plcWal = dynamic_cast<LogCleanWal*>(pWal);
    rs = plcWal->Init();
    EXPECT_EQ(rs.Rc, Code::OK);
    EXPECT_EQ(plcWal->m_minSegmentId, 1);
    EXPECT_EQ(plcWal->m_currentSegmentId, 2);
    EXPECT_NE(plcWal->m_currentSegFd, -1);

    delete pWal;
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
}

TEST(LCWALTest, AppendEntryTest) {
    auto handler = new TestEntryHandler();
    auto loadHandler = std::bind(&TestEntryHandler::OnLoad, handler, std::placeholders::_1);
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    auto pWal = dynamic_cast<LogCleanWal*>(WALFactory::CreateInstance(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler)));
    auto rs = pWal->AppendEntry(nullptr);
    EXPECT_EQ(rs.Rc, Code::Uninited);

    pWal->Init();
    rs = pWal->AppendEntry(nullptr);
    EXPECT_EQ(rs.Rc, Code::Unloaded);
    pWal->Load(loadHandler);

    // size overflow
    auto testEntry = dynamic_cast<TestEntry*>(handler->CreateNewEntryWithContent("overflow"));
    testEntry->SetIllusorySize(LOGCLEAN_WAL_ENTRY_MAX_SIZE + 1);
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
}

TEST(LCWALTest, LoadTest) {
    auto handler = new TestEntryHandler();
    auto loadHandler = std::bind(&TestEntryHandler::OnLoad, handler, std::placeholders::_1);
    utils::FileUtils::RemoveDirectory(WAL_ROOT);
    auto pWal = dynamic_cast<LogCleanWal*>(WALFactory::CreateInstance(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler)));
    // uninitialized
    auto rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::Uninited);
    // no segments
    pWal->Init();
    rs = pWal->Load(loadHandler);
    EXPECT_EQ(rs.Rc, Code::OK);
    EXPECT_EQ(handler->m_vLoadEntries.empty(), true);

    // has entries

}

void prepare_env() {

}