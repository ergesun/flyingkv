/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_SYS_THREAD_POOL_H
#define FLYINGKV_SYS_THREAD_POOL_H

#include <thread>
#include <vector>
#include <unordered_set>
#include <atomic>

#include <sys/sysinfo.h>

#include "../common/blocking-queue.h"
#include "../common/common-def.h"
#include "../utils/common-utils.h"

#include "../sys/spin-lock.h"

namespace flyingkv {
namespace sys {
/**
 * 当前仅支持FIFO调度的线程池。
 * TODO(sunchao): 1、添加调度策略 2、考虑task相关联性，关联task分配到不同的线程 3、cpu亲缘性绑定？再思考，感觉没必要。
 *                4、添加指定的task的wait task、cancel task
 */
template <typename TaskCtxType>
class ThreadPool {
PUBLIC
    struct Task {
        Task() = default;
        /**
         *
         * @param callback 回调函数
         */
        explicit Task(std::function<void(TaskCtxType)> callback) {
            action = callback;
        }

        /**
         *
         * @param callback 回调函数
         * @param ctx 回调传回的上下文
         */
        Task(std::function<void(TaskCtxType)> callback, TaskCtxType ctx) {
            action = callback;
            this->ctx = ctx;
        }

        std::function<void(TaskCtxType)>  action;
        TaskCtxType                       ctx;
    };

    /**
     *
     * @param threads_cnt 线程个数。如果小于等于0则为cpu核数 * 2个。
     */
    explicit ThreadPool(uint32_t threads_cnt = 0) {
        m_iActiveWorkersCnt.store(0);
        threads_cnt = threads_cnt > 0 ? threads_cnt : (uint32_t)get_nprocs() * 2;
        m_vThreadps.reserve(threads_cnt);
        for (uint32_t i = 0; i < threads_cnt; ++i) {
            m_vThreadps.push_back(new std::thread(std::bind(&ThreadPool::proc, this)));
        }
    }

    /**
     * 会join所有threads。
     */
    ~ThreadPool() {
        m_bStopping = true;
        // 添加空的task以防止bq中没有内容无法停止的情况发生。
        Task empty_task;
        auto size = m_vThreadps.size();
        for (size_t i = 0; i < size; ++i) {
            AddTask(empty_task);
        }

        for (auto pt : m_vThreadps) {
            pt->join();
            delete pt;
        }
    }

    /**
     * 添加一个任务到线程池中执行。
     * @param t 添加的task。
     */
    inline void AddTask(Task t) {
        m_bqTasks.Push(t);
    }

    /**
     * 添加一个任务到线程池中执行。
     * @param action
     * @param ctx
     */
    inline void AddTask(std::function<void(TaskCtxType)> action, TaskCtxType ctx) {
        m_bqTasks.Push(Task(action, ctx));
    }

    /**
     * 等待所有的task完成。
     */
    void WaitAll() {
        std::unique_lock<std::mutex> l(m_mtxActiveWorkerCnt);
        while (!m_bqTasks.Empty() || 0 != m_iActiveWorkersCnt.load()) {
            m_cvActiveWorkerCnt.wait(l);
        }
    }

    /**
     * 等待所有都完成或者指定的时间到了。
     * @param duration_since_epoch epoch开始到现在的duration。
     */
    void WaitAllUntilTimeAt(cctime duration_since_epoch) {
        std::unique_lock<std::mutex> l(m_mtxActiveWorkerCnt);
        using namespace std::chrono;
        time_point<system_clock, nanoseconds> tp(nanoseconds(duration_since_epoch.get_total_nsecs()));
        bool first = true;
        while (first && (!m_bqTasks.Empty() && 0 != m_iActiveWorkersCnt.load())) {
            m_cvActiveWorkerCnt.wait_until(l, tp);
            first = false;
        }
    }

    /**
     * 等待所有都完成或者指定的时间到了。
     * @param duration 从现在开始最多等待的持续时间。
     */
    void WaitAllUntilAfter(cctime duration) {
        auto now = sys::cctime::GetCurrentTime();
        now += duration;

        WaitAllUntilTimeAt(now);
    }

PRIVATE
    void proc() {
        while (true) {
            auto task = m_bqTasks.Pop();
            // 此处无需担心m_stopping所判断的成员因为优化导致不能读取到内存数据的情况，
            // 因为BlockingGetQueue::Pop内部有内存屏障作用。
            if (m_bStopping) {
                break;
            }

            if (task.action) {
                m_iActiveWorkersCnt++;
                task.action(task.ctx);
                m_iActiveWorkersCnt--;
            }

            m_cvActiveWorkerCnt.notify_one();
        }
    }

PRIVATE
    bool                           m_bStopping = false;
    std::vector<std::thread*>      m_vThreadps;
    common::BlockingQueue<Task>    m_bqTasks;
    std::atomic<int>               m_iActiveWorkersCnt;
    std::mutex                     m_mtxActiveWorkerCnt;
    std::condition_variable        m_cvActiveWorkerCnt;
}; // class ThreadPool
}  // namespace sys
}  // namespace flyingkv
#endif //FLYINGKV_SYS_THREAD_POOL_H
