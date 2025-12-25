#ifndef LOCK_FREE_QUEUE_H__
#define LOCK_FREE_QUEUE_H__
#include <atomic>
#include <array>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>
#include <limits>

namespace IJK {

template<typename T, std::size_t Size = 1024>
class CLockFreeQueue
{
private:
    // 计算大于等于N的最小2的幂，并检查溢出
    static constexpr const std::size_t getPowerOf2(std::size_t n)
    {
       // 基础检查：如果n已经超过最大值，直接触发编译错误
       if (n > MAX_UINT64 && n == 0)
       {
           return 0; // 会导致后续断言失败
       }
       if ((n & (n - 1)) == 0)
       {
           return n; // 已为2的幂
       }
       std::size_t result = 1;
       // 循环计算下一个2的幂，同时检查溢出
       while (result < n)
       {
           // 检查是否即将溢出（下一次左移会超过最大值）
           if (result > MAX_UINT64 / 2)
           {
               return 0; // 溢出，返回0触发断言
           }
           result <<= 1;
       }
       return result;
    }
    // 64位无符号数的最大值
    static constexpr std::uint64_t MAX_UINT64 = std::numeric_limits<std::uint64_t>::max();
    static constexpr const std::size_t N = getPowerOf2(Size);
    static_assert(N >= 1, "LockFreeQueue: N must be >= 1");
    // 确保N是2的幂，便于后续优化
    static_assert((N & (N - 1)) == 0, "LockFreeQueue: N must be a power of 2");

    // 调整CAS重试策略，减少高并发下的冲突
    static constexpr const int MAX_SPIN_COUNT = 16;        // 最大自旋次数
    static constexpr const int MIN_YIELD_COUNT = 4;        // 开始yield的阈值
    static constexpr const int MIN_SLEEP_COUNT = 8;        // 开始sleep的阈值
    static constexpr const int MIN_SLEEP_NS = 100;         // 最小休眠时间（纳秒）
    static constexpr const int MAX_SLEEP_NS = 100000;      // 最大休眠时间（纳秒）

public:
    CLockFreeQueue() : m_size(0)
    {
        // 初始化节点池
        for (std::size_t i = 0; i < N; ++i)
        {
            m_listPool[i].m_pNext.store(nullptr, std::memory_order_relaxed);
            freePush(&m_listPool[i]);
        }

        // 初始化头指针和尾指针为哨兵节点
        Node* sentinel = freePop();
        if (!sentinel)
        {
            throw std::runtime_error("Failed to initialize lock-free queue");
        }
        sentinel->m_pNext.store(nullptr, std::memory_order_relaxed);
        m_pHead.store(sentinel, std::memory_order_relaxed);
        m_pTail.store(sentinel, std::memory_order_relaxed);
    }

    ~CLockFreeQueue()
    {
        clear();

        // 释放哨兵节点
        Node* sentinel = m_pHead.load(std::memory_order_acquire);
        if (sentinel)
        {
            freePush(sentinel);
        }
    }

    // 入队操作
    void enqueue(const T &value)
    {
        Node *node = allocateNode();
        if (!node)
        {
            throw std::runtime_error("Failed to allocate node for enqueue");
        }

        node->m_Data = value;
        node->m_pNext.store(nullptr, std::memory_order_release);

        int spinCount = 0;
        Node* oldTail = nullptr;

        while (true)
        {
            oldTail = m_pTail.load(std::memory_order_acquire);
            Node* next = oldTail->m_pNext.load(std::memory_order_acquire);

            // 如果tail已被其他线程移动，重新获取，并自旋等待一定时间
            if (oldTail != m_pTail.load(std::memory_order_acquire))
            {
                spinWait(spinCount++);
                continue;
            }

            // 帮助推进尾节点
            // 如果tail是尾节点，但是tail->next不为空，说明其他线程修改了tail的next但是没有推进tail，这里帮助推进（更新）tail，并自旋等待一定时间
            if (next != nullptr)
            {
                // 如果尾指针未被推进（m_pTail==oldTail），则进行推进（m_pTail=next），并发布当前线程此操作和此前的修改；如果推进成功，返回true
                // 如果尾指针已被推进（m_pTail!=oldTail），则更新旧尾指针（oldTail=next），获取其他线程的修改，返回false
                m_pTail.compare_exchange_weak(oldTail, next,
                                             std::memory_order_release,
                                             std::memory_order_acquire);
                spinWait(spinCount++);
                continue;
            }

            // 这里可以确保m_pTail（oldTail）是尾节点，next为空
            // 尝试将新节点链接到tail后面
            // 更新尾节点的next指针
            if (oldTail->m_pNext.compare_exchange_weak(next, node,
                                                     std::memory_order_release,
                                                     std::memory_order_acquire))
            {
                // 成功链接，尝试更新tail，退出函数
                // 注：这里存在伪失败的可能，推进尾节点失败（但是尾节点的next指针已经更新过了），所以上面需要增加帮助推进尾节点的步骤
                m_pTail.compare_exchange_weak(oldTail, node,
                                             std::memory_order_release,
                                             std::memory_order_acquire);
                m_size.fetch_add(1, std::memory_order_release);
                return;
            }
            // 更新尾节点next指针失败，自旋等待之后重新执行
            spinWait(spinCount++);
        }
    }

    // 出队操作，返回是否成功
    bool dequeue(T &value)
    {
        int spinCount = 0;
        Node* oldHead = nullptr;

        while (true)
        {
            oldHead = m_pHead.load(std::memory_order_acquire);
            Node* oldTail = m_pTail.load(std::memory_order_acquire);
            Node* next = oldHead->m_pNext.load(std::memory_order_acquire);

            // 如果head已被移动，重新获取
            if (oldHead != m_pHead.load(std::memory_order_acquire))
            {
                spinWait(spinCount++);
                continue;
            }

            // 队列为空
            if (oldHead == oldTail)
            {
                if (next == nullptr)
                {
                    return false; // 确实为空
                }
                // tail滞后，帮助推进
                m_pTail.compare_exchange_weak(oldTail, next,
                                             std::memory_order_release,
                                             std::memory_order_acquire);
                spinWait(spinCount++);
                continue;
            }

            // 尝试移动head指针
            if (next == nullptr)
            {
                spinWait(spinCount++);
                continue;
            }

            // 提前读取数据，减少CAS失败时的开销
            value = next->m_Data;

            if (m_pHead.compare_exchange_weak(oldHead, next,
                                             std::memory_order_release,
                                             std::memory_order_acquire))
            {
                // 成功出队，回收旧节点
                freePush(oldHead);
                m_size.fetch_sub(1, std::memory_order_release);
                return true;
            }

            spinWait(spinCount++);
        }
    }

    // 获取队列大小（近似值，高并发下不保证绝对准确）
    uint64_t size() const
    {
        return m_size.load(std::memory_order_acquire);
    }

    // 清空队列
    void clear()
    {
        T dummy;
        int count = 0;
        while (dequeue(dummy))
        {
            count++;
        }
        m_size.store(0, std::memory_order_release);
    }

private:
    struct Node
    {
        T                   m_Data;
        std::atomic<Node *> m_pNext;
    };

    // 节点池和指针，使用缓存行对齐减少false sharing
    alignas(64) std::array<Node, N> m_listPool;
    alignas(64) std::atomic<Node *> m_pFreeHead {nullptr};
    alignas(64) std::atomic<Node *> m_pHead; // 头节点
    alignas(64) std::atomic<Node *> m_pTail; // 尾节点
    alignas(64) std::atomic<uint64_t> m_size; // 实际元素个数

    // 自旋等待策略，根据重试次数采用不同策略
    void spinWait(int count) const
    {
        if (count < MIN_YIELD_COUNT) {
            // 次数较少时，使用空操作自旋
            for (int i = 0; i < (1 << count); ++i)
            {
                // 内存操作屏障
                std::atomic_signal_fence(std::memory_order_acquire);
            }
        }
        else if (count < MIN_SLEEP_COUNT)
        {
            // 中等次数，使用yield
            std::this_thread::yield();
        }
        else {
            // 次数较多，使用休眠
            const int level = count - MIN_SLEEP_COUNT;
            const int sleepNs = std::min(MIN_SLEEP_NS * (1 << level), MAX_SLEEP_NS);
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    }

    // 从空闲列表获取节点
    Node* allocateNode()
    {
        int spinCount = 0;
        while (true)
        {
            Node* node = freePop();
            if (node)
            {
                return node;
            }

            // 空闲列表为空，尝试出队一个元素来获取节点
            T dummy;
            if (dequeue(dummy))
            {
                continue; // 成功出队，再次尝试获取空闲节点
            }

            // 队列也为空，只能等待
            spinWait(spinCount++);
        }
    }

    // 将节点加入空闲列表
    void freePush(Node *node)
    {
        node->m_pNext.store(nullptr, std::memory_order_relaxed);
        int spinCount = 0;
        Node* oldFreeHead = m_pFreeHead.load(std::memory_order_acquire);

        do {
            node->m_pNext.store(oldFreeHead, std::memory_order_relaxed);
            if (m_pFreeHead.compare_exchange_weak(oldFreeHead, node,
                                                  std::memory_order_release,
                                                  std::memory_order_acquire))
            {
                return;
            }
            spinWait(spinCount++);
        } while (true);
    }

    // 从空闲列表取出节点
    Node* freePop()
    {
        int spinCount = 0;
        Node* oldFreeHead = m_pFreeHead.load(std::memory_order_acquire);

        while (oldFreeHead) {
            Node* next = oldFreeHead->m_pNext.load(std::memory_order_acquire);
            if (m_pFreeHead.compare_exchange_weak(oldFreeHead, next,
                                                  std::memory_order_release,
                                                  std::memory_order_acquire))
            {
                return oldFreeHead;
            }
            spinWait(spinCount++);
            oldFreeHead = m_pFreeHead.load(std::memory_order_acquire);
        }

        return nullptr;
    }

};

// 关键：在类外定义静态常量成员（模板特化定义）
// 否则报错：test_lock_free_queue.cpp:(.text._ZNK3IJK14CLockFreeQueueIiLm4194304EE8spinWaitEi[_ZNK3IJK14CLockFreeQueueIiLm4194304EE8spinWaitEi]+0x77): undefined reference to `IJK::CLockFreeQueue<int, 4194304ul>::MAX_SLEEP_NS'
// collect2: error: ld returned 1 exit status

template<typename T, std::size_t N>
constexpr const int CLockFreeQueue<T, N>::MAX_SPIN_COUNT;

template<typename T, std::size_t N>
constexpr const int CLockFreeQueue<T, N>::MIN_YIELD_COUNT;

template<typename T, std::size_t N>
constexpr const int CLockFreeQueue<T, N>::MIN_SLEEP_COUNT;

template<typename T, std::size_t N>
constexpr const int CLockFreeQueue<T, N>::MIN_SLEEP_NS;

template<typename T, std::size_t N>
constexpr const int CLockFreeQueue<T, N>::MAX_SLEEP_NS;

}

#endif
