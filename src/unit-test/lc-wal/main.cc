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

using namespace flyingkv;
using namespace flyingkv::wal;
using namespace flyingkv::waltest;

#define INVALID_WAL_ROOT  "/invalid_wal_root"
#define INVALID_WAL_TYPE  "invalid_wal_type"
#define WAL_TYPE  "log-clean"
#define WAL_ROOT  "/tmp/wal-test"

void prepare_env();


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

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
    auto handler = new TestEntryHandler();

    // - invalid root
    IWal *pWal = WALFactory::CreateInstance(WAL_TYPE, INVALID_WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
    auto rs = pWal->Init();
    EXPECT_EQ(rs.Rc, Code::FileSystemError);

    pWal = WALFactory::CreateInstance(WAL_TYPE, WAL_ROOT, std::bind(&TestEntryHandler::CreateNewEntry, handler));
    auto plcWal = dynamic_cast<LogCleanWal*>(pWal);
    rs = plcWal->Init();

    plcWal->create_trunc_ok_flag();

    // has ok flag
    auto id = plcWal->get_uncompleted_trunc_segments_max_id();
    EXPECT_EQ(LOGCLEAN_WAL_INVALID_SEGMENT_ID, id);
}

TEST(LCWALTest, IOTest) {
}

void prepare_env() {

}