#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>

namespace nv
{
    template<typename T>
    class ConcurrentQueue
    {
    public:
        void        Push(T&& val);
        void        Push(const T& val);
        void        Push(T& val);

        T           Pop(bool wait = false);
        void        Pop(T& val, bool wait = false);
        void        PopUnsafe(T& val) { val = std::move(mQueue.front()); mQueue.pop(); }
        const T&    Peek();
        size_t      Size() const { return mQueue.size(); }

        bool        IsEmpty() const;
        void        Lock() { mMutex.lock(); }
        void        Unlock() { mMutex.unlock(); }   
    private:
        using UniqueLock = std::unique_lock<std::mutex>;

        std::mutex              mMutex;
        std::condition_variable mConditionVar;
        std::queue<T>           mQueue;
        std::atomic_bool        mbIsEmpty = true;
    };

    template<typename T>
    inline void ConcurrentQueue<T>::Push(T&& val)
    {
        {
            UniqueLock lock(mMutex);
            mQueue.push(val);
            lock.unlock();
        }

        mbIsEmpty.store(false);
        mConditionVar.notify_one();
    }

    template<typename T>
    inline void ConcurrentQueue<T>::Push(const T& val)
    {
        {
            UniqueLock lock(mMutex);
            mQueue.push(val);
            lock.unlock();
        }

        mbIsEmpty.store(false);
        mConditionVar.notify_one();
    }

    template<typename T>
    inline void ConcurrentQueue<T>::Push(T& val)
    {
        {
            UniqueLock lock(mMutex);
            mQueue.push(std::move(val));
            lock.unlock();
        }

        mbIsEmpty.store(false);
        mConditionVar.notify_one();
    }

    template<typename T>
    inline T ConcurrentQueue<T>::Pop(bool wait)
    {
        UniqueLock lock(mMutex);
        while (IsEmpty() && wait)
        {
            mConditionVar.wait(lock);
        }

        if (IsEmpty())
            return T();

        auto val = mQueue.front();
        mQueue.pop();
        mbIsEmpty.store(mQueue.empty());
        return val;
    }

    template<typename T>
    inline void ConcurrentQueue<T>::Pop(T& val, bool wait)
    {
        UniqueLock lock(mMutex);
        while (IsEmpty() && wait)
        {
            mConditionVar.wait(lock);
        }

        if (IsEmpty())
            return;

        val = std::move(mQueue.front());
        mQueue.pop();
        mbIsEmpty.store(mQueue.empty());
    }

    template<typename T>
    inline const T& ConcurrentQueue<T>::Peek()
    {
        return mQueue.front();
    }

    template<typename T>
    inline bool ConcurrentQueue<T>::IsEmpty() const
    {
        return mbIsEmpty.load();
    }
}