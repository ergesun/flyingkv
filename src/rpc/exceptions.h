/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_RPC_EXCEPTIONS_H
#define FLYINGKV_RPC_EXCEPTIONS_H

#include <exception>
#include <cstdint>
#include <string>
#include <sstream>

namespace flyingkv {
namespace rpc {
class RpcException : public std::exception {
PUBLIC
    RpcException() = default;
    RpcException(std::string &what) : m_sWhat(what) {}
    RpcException(std::string &&what) : m_sWhat(std::move(what)) {}

PROTECTED
    std::string m_sWhat;
};

class BadRpcException : public RpcException {
PUBLIC
    explicit BadRpcException(uint16_t code, std::string &&rpcName) : m_iCode(code), m_sRpcName(std::move(rpcName)) {
        std::stringstream ss;
        ss << "Cannot found registered rpc " << m_sRpcName << ".";
        m_sWhat = ss.str();
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }

    uint16_t code() const {
        return m_iCode;
    }

PRIVATE
    uint16_t m_iCode;
    std::string m_sRpcName;
};

class BadRpcHandlerIdException : public RpcException {
PUBLIC
    BadRpcHandlerIdException(uint16_t code, uint16_t handlerId) : m_iCode(code), m_iHandlerId(handlerId) {
        std::stringstream ss;
        ss << "Bad handler id " << m_iHandlerId << ".";
        m_sWhat = ss.str();
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }

    uint16_t code() const {
        return m_iCode;
    }

    uint16_t handler_id() const {
        return m_iHandlerId;
    }

PRIVATE
    uint16_t m_iCode;
    uint16_t m_iHandlerId;

};

class RpcClientInternalException : public RpcException {
PUBLIC
    RpcClientInternalException() {
        m_sWhat = "Rpc client internal error!";
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }
};

class RpcClientSendTimeoutException : public RpcException {
PUBLIC
    RpcClientSendTimeoutException() {
        m_sWhat = "Rpc client send timeout!";
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }
};

class RpcBrokenPipeException : public RpcException {
PUBLIC
    RpcBrokenPipeException() {
        m_sWhat = "Rpc pipe is broken!";
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }
};

class RpcMessageCorruptException : public RpcException {
PUBLIC
    RpcMessageCorruptException(uint16_t handlerId) {
        std::stringstream ss;
        ss << "Cannot parse sent message error for handler id " << handlerId << ".";
        m_sWhat = ss.str();
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }
};

class RpcServerInternalException : public RpcException {
PUBLIC
    RpcServerInternalException() {
        m_sWhat = "Rpc peer server internal error! Maybe oom.";
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }
};

class RpcClientIsBusyException : public rpc::RpcException {
PUBLIC
    RpcClientIsBusyException() {
        m_sWhat = "Cannot push message into sent msg queue, the sent msg queue is full!";
    }

    const char *what() const noexcept override {
        return m_sWhat.c_str();
    }
};
}
}

#endif //FLYINGKV_RPC_EXCEPTIONS_H
