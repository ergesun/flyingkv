/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_COMMON_REFERENCE_COUNTER_H
#define FLYINGKV_COMMON_REFERENCE_COUNTER_H

namespace flyingkv {
namespace common {
/**
 * Thread-safe.
 * 维护一个引用计数的类。
 */
class ReferenceCounter {
PUBLIC
    explicit ReferenceCounter(uint32_t ref) : m_iRef(ref) {}
    virtual ~ReferenceCounter() = default;

    /**
     * 添加一个引用计数。
     */
    virtual void AddRef();

    /**
     * 释放一个引用计数。
     */
    virtual void Release();

PROTECTED
    /**
     * 指定一个引用计数。
     * @param val
     */
    void SetRef(int32_t val);

    /**
     * 获取当前引用计数。
     * @return 引用计数。
     */
    int32_t GetRef();

PRIVATE
    int32_t        m_iRef = 0;
}; // class ReferenceCounter
} // namespace common
} // namespace flyingkv

#endif //FLYINGKV_COMMON_REFERENCE_COUNTER_H
