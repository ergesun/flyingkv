/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_TEST_SEND_MESSAGE_H
#define MINIKV_TEST_SEND_MESSAGE_H

#include "../../../common/common-def.h"
#include "../../../net/snd-message.h"

using namespace minikv::net;

namespace minikv {
namespace test {
class TestSndMessage : public net::SndMessage {
public:
    TestSndMessage(sys::MemPool *mp, net::net_peer_info_t &&socketInfo, std::string msg);

#ifdef WITH_MSG_ID
    TestSndMessage(sys::MemPool *mp, net::net_peer_info_t &&socketInfo, net::Message::Id id, std::string msg);
#endif

protected:
    uint32_t getDerivePayloadLength() override;
    void encodeDerive(common::Buffer *b) override;

private:
    std::string    m_str;
};
}
}

#endif //MINIKV_TEST_SEND_MESSAGE_H
