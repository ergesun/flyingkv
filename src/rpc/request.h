/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_RPC_REQUEST_H
#define FLYINGKV_RPC_REQUEST_H

#include <memory>

#include "../net/snd-message.h"

namespace google {
namespace protobuf {
class Message;
}
}

namespace flyingkv {
namespace rpc {
class RpcRequest : public net::SndMessage {
PUBLIC
    RpcRequest(sys::MemPool *mp, net::net_peer_info_t &&peerInfo,
               uint16_t handlerId, std::shared_ptr<google::protobuf::Message> &&msg) :
        net::SndMessage(mp, std::move(peerInfo)), m_iHandlerId(handlerId), m_sspMsg(std::move(msg)) {}

    RpcRequest(sys::MemPool *mp, net::Message::Id id, net::net_peer_info_t &&peerInfo,
               uint16_t handlerId, std::shared_ptr<google::protobuf::Message> &&msg) :
        net::SndMessage(mp, std::move(peerInfo), id), m_iHandlerId(handlerId), m_sspMsg(std::move(msg)) {}

PROTECTED
    uint32_t getDerivePayloadLength() override;
    void encodeDerive(common::Buffer *b) override;

PRIVATE
    uint16_t                                   m_iHandlerId;
    std::shared_ptr<google::protobuf::Message> m_sspMsg;
};
} // namespace rpc
} // namespace flyingkv

#endif //FLYINGKV_RPC_REQUEST_H
