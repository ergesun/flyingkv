/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_TCP_SERVER_TEST_CASE_H
#define FLYINGKV_TCP_SERVER_TEST_CASE_H

#include <memory>

#include "../../../../net/notify-message.h"

namespace flyingkv {
namespace sys {
    class MemPool;
}
namespace net {
    class ISocketService;
}

namespace test {
class TcpServerTest {
public:
    static void Run();

private:
    static void recv_msg(std::shared_ptr<flyingkv::net::NotifyMessage> sspNM);
    static flyingkv::net::ISocketService     *s_ss;
    static flyingkv::sys::MemPool          *m_mp;
};
}
}

#endif //FLYINGKV_TCP_SERVER_TEST_CASE_H
