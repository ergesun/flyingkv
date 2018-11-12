/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_CLIENT_CONFIG_H
#define FLYINGKV_CLIENT_CONFIG_H

#include <string>

namespace flyingkv {
namespace client {
struct ClientConfig {
    std::string ServerIp;
    uint16_t    ServerPort;
    uint16_t    ClientPort;
    uint32_t    ConnectTimeoutUs;
    uint32_t    ProcessMessageTimeoutMs;
    uint16_t    RpcIOThreadsCnt;

    ClientConfig(const std::string &sIp, uint16_t sPort, uint16_t cPort, uint32_t connTimeoutUs, uint32_t procMsgTimeoutMs,
                uint16_t rpcIOThreadsCnt) : ServerIp(sIp), ServerPort(sPort), ClientPort(cPort), ConnectTimeoutUs(connTimeoutUs),
                                            ProcessMessageTimeoutMs(procMsgTimeoutMs), RpcIOThreadsCnt(rpcIOThreadsCnt) {}
};
}
}

#endif //FLYINGKV_CLIENT_CONFIG_H
