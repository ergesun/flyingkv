/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <exception>
#include <map>

#include <sys/stat.h>
#include <csignal>
#include <condition_variable>

#include "../common/common-def.h"
#include "../common/errors.h"
#include "../utils/file-utils.h"
#include "../common/server-gflags-config.h"
#include "../common/global-vars.h"
#include "../common/iservice.h"
#include "../common/ikv-common.h"
#include "../kv/mini-kv/mini-kv.h"

#include "./rpc/kv-rpc-sync-server.h"

using namespace flyingkv;

using std::string;
using std::stringstream;
using std::map;

using utils::FileUtils;

common::IService *g_pKV  = nullptr;
common::IService *g_pRpc = nullptr;

bool              g_bStopped = true;
std::mutex        g_m;
std::condition_variable g_cv;

void init_gflags_glog(int *argc, char ***argv) {
    gflags::ParseCommandLineFlags(argc, argv, true);
    google::InitGoogleLogging((*argv)[0]);
    FLAGS_log_dir = FLAGS_glog_dir;
    if (-1 == FileUtils::CreateDirPath(FLAGS_log_dir.c_str(), 0775)) {
        THROW_CREATE_DIR_ERR();
    }

    FLAGS_log_prefix = FLAGS_glog_prefix;
    FLAGS_max_log_size = FLAGS_max_glog_size;
    FLAGS_logbufsecs = FLAGS_glogbufsecs;
    FLAGS_minloglevel = FLAGS_mingloglevel;
    FLAGS_stop_logging_if_full_disk = FLAGS_stop_glogging_if_full_disk;
    FLAGS_logtostderr = FLAGS_glogtostderr;
    FLAGS_alsologtostderr = FLAGS_alsoglogtostderr;
    FLAGS_colorlogtostderr = FLAGS_colorglogtostderr;
}

void uninit_gflags_glog() {
    google::ShutdownGoogleLogging();
    gflags::ShutDownCommandLineFlags();
}

void startup() {
    std::unique_lock<std::mutex> l(g_m);
    // TODO(sunchao): use factory
    auto pKV = new kv::MiniKV(FLAGS_wal_type, FLAGS_checkpoint_type, FLAGS_wal_dir,
                                FLAGS_checkpoint_dir, FLAGS_acc_conf_path);
    g_pKV = pKV;
    if (!g_pKV->Start()) {
        LOGFFUN << "start kv service is failure.";
    }
    auto pRpc = new server::KVRpcServerSync(pKV, uint16_t(FLAGS_rpc_server_threads_cnt),
                                            uint16_t(FLAGS_rpc_io_threads_cnt), uint16_t(FLAGS_rpc_port));
    g_pRpc = pRpc;
    if (!g_pRpc->Start()) {
        LOGFFUN << "start rpc service is failure.";
    }

    g_bStopped = false;
}

void stop() {
    if (g_bStopped) {
        return;
    }

    std::unique_lock<std::mutex> l(g_m);
    if (!g_pRpc->Stop()) {
        LOGEFUN << "rpc stop is failure.";
    }

    if (!g_pKV->Stop()) {
        LOGFFUN << "kv stop is failure.";
    }

    g_bStopped = true;
}

void waitStop() {
    std::unique_lock<std::mutex> l(g_m);
    while (!g_bStopped) {
        g_cv.wait(l);
    }
}

void signal_handler(int sig) {
    static const map<int, string> what_sig = map<int, string>{
        {SIGHUP, "SIGHUP"},
        {SIGINT, "SIGINT"},
        {SIGTERM, "SIGTERM"},
        {SIGPIPE, "SIGPIPE"},
        {SIGALRM, "SIGALRM"}
    };

    auto iter = what_sig.find(sig);
    if (iter != what_sig.end()) {
        LOGWFUN << "catch " << iter->second << " signal";
    } else {
        LOGWFUN << "default catch " << sig << " signal";
    }

    switch (sig) {
        case SIGHUP: { // TODO:reload conf?
            break;
        }
        case SIGINT:
        case SIGTERM: {
            stop();
            break;
        }
        default: {
            break;
        }
    }
}

void register_signal() {
    struct sigaction newSigaction;
    newSigaction.sa_handler = signal_handler;
    sigemptyset(&newSigaction.sa_mask);
    newSigaction.sa_flags = 0;
    sigaction(SIGHUP, &newSigaction, nullptr); /* catch hangup signal */
    sigaction(SIGINT, &newSigaction, nullptr); /* catch term signal */
    sigaction(SIGTERM, &newSigaction, nullptr); /* catch interrupt signal */
    sigaction(SIGPIPE, &newSigaction, nullptr); /* catch pipe signal */
    sigaction(SIGALRM, &newSigaction, nullptr); /* catch alarm signal */
}

int
main(int argc, char *argv[])
try {
    if (argc != 2) {
        LOGEFUN << "you must just start app with --flagfile=\"xxx\" args";
        return -1;
    }

    umask(0);
    init_gflags_glog(&argc, &argv);
    flyingkv::common::initialize();
    startup();
    register_signal();

    waitStop();
    flyingkv::common::uninitialize();
    uninit_gflags_glog();

    return 0;
} catch (std::exception &ex) {
    LOG(FATAL) << ex.what();
    uninit_gflags_glog();
} catch (std::exception *ex) {
    LOG(FATAL) << ex->what();
    uninit_gflags_glog();
} catch (...) {
    LOG(FATAL) << "Unknown exception occur!";
    uninit_gflags_glog();
}
