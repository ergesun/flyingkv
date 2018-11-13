/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#include <iostream>

#include "../common/global-vars.h"

#include "../client/flyingkv-client.h"
#include "../codegen/kvrpc.pb.h"

using namespace flyingkv;
using namespace flyingkv::client;

int main(int argc, char **argv) {
    std::string sIP = "127.0.0.1";
    ClientConfig cc = {.ServerIp = sIP, .ServerPort = 2210, .ClientPort = 2289, .ConnectTimeoutUs = 100000,
                        .ProcessMessageTimeoutMs = 10000000, .RpcIOThreadsCnt = 1};

    auto pClient = new FlyingKVClient(cc);
    pClient->Start();
    std::cout << "start get " << std::endl;
    auto getRs = pClient->Get("sun");
    std::cout << "getRs.code " << getRs->rc() << ", getRs.value = " << getRs->entry().value() << std::endl;

    std::cout << "start scan " << std::endl;
    auto scanRs = pClient->Scan("sun", true, protocol::SortOrder::Asc, 1);
    std::cout << "scanRs.code " << scanRs->rc() << ", scanRs.count = " << scanRs->entries().size() << std::endl;

    std::cout << "start put " << std::endl;
    auto putRs = pClient->Put("sun", "chao");
    std::cout << "putRs.code " << putRs->rc() << std::endl;

    std::cout << "start get " << std::endl;
    getRs = pClient->Get("sun");
    std::cout << "getRs.code " << getRs->rc() << ", getRs.value = " << getRs->entry().value() << std::endl;

    std::cout << "start scan " << std::endl;
    scanRs = pClient->Scan("sun", true, protocol::SortOrder::Asc, 1);
    std::cout << "scanRs.code " << scanRs->rc() << ", scanRs.count = " << scanRs->entries().size() << std::endl;

    std::cout << "start delete " << std::endl;
    auto delRs = pClient->Delete("sun");
    std::cout << "delRs.code " << delRs->rc() << std::endl;

    std::cout << "start get " << std::endl;
    getRs = pClient->Get("sun");
    std::cout << "getRs.code " << getRs->rc() << ", getRs.value = " << getRs->entry().value() << std::endl;

    std::cout << "start scan " << std::endl;
    scanRs = pClient->Scan("sun", true, protocol::SortOrder::Asc, 1);
    std::cout << "scanRs.code " << scanRs->rc() << ", scanRs.count = " << scanRs->entries().size() << std::endl;
}
