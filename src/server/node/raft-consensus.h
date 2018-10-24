/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_SERVER_RAFT_CONSENSUS_H
#define MINIKV_SERVER_RAFT_CONSENSUS_H

#include <cstdint>

#include "common-def.h"
#include "iservice.h"
#include "../../sys/timer.h"
#include "../../sys/cctime.h"
#include "../../sys/random.h"
#include "../../sys/rw-mutex.h"
#include "../../common/rf-server.h"

#include "../../kv/ikv-handler.h"
#include "iservice-rpc-callback-handler.h"

#define RFLOG_DIR          "rflogs"
#define RFLOG_FILE_NAME    "rflog"
#define RFSTATE_FILE_NAME  "rfstate"

//         RFSTATE_MAGIC_NO        r f s t
#define    RFSTATE_MAGIC_NO      0x72667374
#define    RFSTATE_MAGIC_NO_LEN  4

using namespace google;
using namespace minikv::rpc;

/**
 * 没有采用相对简单并且peer少的时候线程上线文切换少的方式比如说一个peer一个线程处理的方式，而是采用流水线的形式，
 * 一是为了扩展(适配multi raft或者压根就是peer较多呢？)，二是为了更难以便考验自己。
 * TODO(sunchao): 考虑有没有必要摒弃流水线模式，改成一个peer独占一个线程玩心跳以及选举。
 */
namespace minikv {
namespace rflog {
class IRfLogger;
}
namespace server {
class ServerRpcService;
class ElectorManagerService;
class RaftConsensus : public IService, public IKVRpcRpcHandler, public IServiceRpcCallbackHandler {
public:
    RaftConsensus();
    ~RaftConsensus() override;

    bool Start() override;
    bool Stop() override;

    // IKVRpcRpcHandler IF
    SP_PB_MSG OnAppendEntries(SP_PB_MSG sspMsg) override;
    SP_PB_MSG OnRequestVote(SP_PB_MSG sspMsg) override;
    void OnRecvRpcReturnResult(std::shared_ptr<net::NotifyMessage> sspNM) override;

    // IServiceRpcCallbackHandler IF
    void OnMessageSent(ARpcClient::SentRet &&sentRet) override;
    void OnRecvRequestVoteRetRes(protocol::RequestVoteResponse *resp) override;
    void OnRecvAppendEntriesRetRes(protocol::AppendEntriesResponse *resp) override;

private:
    void initialize();
    // TODO(sunchao): move to logger
    void save_rf_state();
    void start_new_election();
    void prepare_new_election_env();
    inline void subscribe_leader_hb_timer_tick();
    /**
     * leader心跳超时。
     */
    inline void on_leader_hb_timeout(void *ctx);
    inline void subscribe_request_vote_timeout_tick();
    /**
     * 广播选举请求到收到major响应超时。
     * @param ctx
     */
    void on_request_vote_timeout(void *ctx);
    void broadcast_request_vote(const std::map<uint32_t, common::RfServer> &otherSrvs);

    bool is_valid_msg_id(const net::RcvMessage *rm);

private:
    bool                           m_bStopped                    = true;
    // 先用大粒度锁锁环境。
    // TODO(sunchao): 优化并发锁的控制。
    sys::RwMutex                   m_envRwLock;
    NodeRoleType                   m_roleType                    = NodeRoleType::Follower;
    uint32_t                       m_iCurrentTerm                = 0;
    uint32_t                       m_iMyId                       = 0;
    /**
     * zero is no server.
     */
    uint32_t                                m_iVoteFor                    = 0;
    std::string                             m_sRfStateFilePath;
    rflog::IRfLogger                       *m_pRfLogger                   = nullptr;
    int32_t                                 m_iRfStateFd                  = -1;
    sys::Timer                             *m_pTimer                      = nullptr;
    sys::Timer::Event                       m_monitorLeaderHbTimerEvent;
    sys::Timer::EventId                     m_monitorLeaderHbTimerEventId;
    sys::Timer::TimerCallback               m_moniterLeaderHbTimerCb;
    sys::Timer::Event                       m_monitorRVTimeoTimerEvent;
    sys::Timer::EventId                     m_monitorRVTimeoTimerEventId;
    sys::Timer::TimerCallback               m_moniterRVTimeoTimerCb;
    sys::Random                             m_random;
    uint32_t                                m_iRequestVoteTimeoutInterval = 0;
    sys::RwMutex                            m_sentMsgsIdRwMtx;
    std::unordered_set<net::Message::Id>    m_sentMsgsId;

    /**
     * 关联关系，无需本类释放。
     */
    ElectorManagerService                  *m_pElectorManager             = nullptr;
    ServerRpcService                       *m_pSrvRpcService              = nullptr;
}; // class RaftConsensus
} // namespace server
} // namespace minikv

#endif //MINIKV_SERVER_RAFT_CONSENSUS_H