#include "pch.h"

#include <Engine/JobSystem.h>
#include <Engine/Job.h>

#include <Lib/Pool.h>
#include <Lib/ConcurrentQueue.h>
#include <Lib/Format.h>

#include <Debug/Profiler.h>

#include <Windows.h>

namespace nv::jobs
{
    IJobSystem* gJobSystem = nullptr;

#if NV_PLATFORM_WINDOWS
    static void SetThreadAffinity_Win32(const wchar_t* name, HANDLE handle, uint32_t coreIndex)
    {
        DWORD_PTR affinityMask = 1ull << coreIndex;
        DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);

        HRESULT hr = SetThreadDescription(handle, name);
        SetThreadPriority(handle, THREAD_PRIORITY_ABOVE_NORMAL);
    }
#endif

    class JobSystem : public IJobSystem
    {
    public:
        JobSystem(uint32_t threadCount):
            mThreadCount(threadCount),
            mIsRunning(false),
            mJobs(),
            mQueue(),
            mConditionVar(),
            mMutex(),
            mThreads(threadCount)
        {
            mJobs.Init();
        }

        void Poll()
        {
            mConditionVar.notify_one();
            std::this_thread::yield();
        }

        Handle<Job> AllocateJob(Job&& job)
        {
            std::unique_lock<NV_LOCKABLE(std::mutex)> lock(mJobPoolMutex);
            Handle<Job> handle = mJobs.Create();
            auto mInstance = mJobs.Get(handle);
            *mInstance = std::move(job);
            mCurrentJobs.push_back(handle);
            return handle;
        }

        void RemoveJob(Handle<Job> handle)
        {
            std::unique_lock<NV_LOCKABLE(std::mutex)> lock(mJobPoolMutex);
            mJobs.Remove(handle);
        }

        void GarbageCollect() override
        {
            std::unique_lock<NV_LOCKABLE(std::mutex)> lock(mJobPoolMutex);
            auto it = mCurrentJobs.begin();
            while (it != mCurrentJobs.end())
            {
                auto* pJob = mJobs.Get(*it);
                if (!pJob || pJob->IsFinished())
                {
                    mJobs.Remove(*it);
                    it = mCurrentJobs.erase(it);
                }
                else
                    ++it;
            }
        }

        virtual Handle<Job> Enqueue(Job&& job) override
        {
            Handle<Job> handle = AllocateJob(std::move(job));
            mQueue.Push(handle);
            mConditionVar.notify_one();
            return handle;
        }

        virtual void Wait(Handle<Job> handle) override
        {
            if (!mJobs.IsValid(handle)) 
                return;

            auto job = mJobs.GetAsDerived(handle);
            while (!job->IsFinished())
            {
                Poll();
            }
        }

        virtual void Wait() override
        {
            while (!mQueue.IsEmpty())
            {
                Poll();
            }

            mJobs.Destroy();
        }

        virtual bool IsFinished(Handle<Job> handle) override
        {
            const bool bIsValid = mJobs.IsValid(handle);
            if (!bIsValid)
                return true;

            const Job* const pJob = mJobs.GetAsDerived(handle);
            if (!pJob || pJob->IsFinished())
                return true;

            return false;
        }

        void Stop()
        {
            mIsRunning = false;
            mConditionVar.notify_all(); // Unblock all threads and stop
        }

        void Start()
        {
            SetThreadAffinity_Win32(L"NVMainThread", GetCurrentThread(), 0);
            mIsRunning = true;

            auto worker = [&]()
            {
                const auto threadId = std::hash<std::jthread::id>{}(std::this_thread::get_id());
                const auto jobThreadName = nv::Format("NovaWorker-{:x}", threadId);
                NV_THREAD(jobThreadName.c_str());
                while (mIsRunning)
                {
                    NV_FRAME("NovaJobThread");
                    {
                        NV_EVENT("JobSys/WaitForNewJob");
                        std::unique_lock<std::mutex> lock(mMutex);
                        if (!mIsRunning)
                            break;

                        mConditionVar.wait(lock, [&]
                            {
                                return !mIsRunning || !mQueue.IsEmpty(); // TODO: Condition may not be needed as Execute() notifies this wait() anyway?
                            });
                    }

                    while (!mQueue.IsEmpty())
                    {
                        auto jobHandle = mQueue.Pop();
                        auto job = mJobs.Get(jobHandle);
                        if (job)
                        {
                            job->Invoke();
                            job->mIsFinished.store(true);
                        }
                    }
                }
            };

            for (uint32_t i = 0; i < mThreadCount; ++i)
            {
                mThreads[i] = std::jthread(worker);

#ifdef _WIN32 // Credits: https://wickedengine.net/2018/11/24/simple-job-system-using-standard-c/#comments
                // Do Windows-specific thread setup:
                HANDLE handle = (HANDLE)mThreads[i].native_handle();
                SetThreadAffinity_Win32(L"Nova-Worker", handle, i);
#endif // _WIN32
            }
        }

        ~JobSystem()
        {
            Stop();
            Wait();
            for (auto& thread : mThreads)
                thread.join();
            mJobs.Destroy();
        }

    private:
        uint32_t                        mThreadCount;
        std::atomic_bool                mIsRunning;
        Pool<Job>                       mJobs;
        ConcurrentQueue<Handle<Job>>    mQueue;
        std::condition_variable         mConditionVar;
        std::mutex                      mMutex;
        NV_MUTEX(std::mutex,            mJobPoolMutex);
        std::vector<std::jthread>       mThreads;
        std::vector<Handle<Job>>        mCurrentJobs;
    };

    void InitJobSystem(uint32_t threads)
    {
        gJobSystem = Alloc<JobSystem>(SystemAllocator::gPtr, threads);
        auto jobSystem = (JobSystem*)gJobSystem;
        jobSystem->Start();
    }

    void DestroyJobSystem()
    {
        auto jobSystem = (JobSystem*)gJobSystem;
        jobSystem->Stop();
        Free<JobSystem>(jobSystem);
    }

    Handle<Job> Execute(Job::Fn&& job)
    {
        return gJobSystem->Enqueue(job);
    }

    Handle<Job> Execute(Job::Fn&& job, void* context)
    {
        Job j(std::move(job), context);
        return gJobSystem->Enqueue(std::move(j));
    }

    void Wait(Handle<Job> handle)
    {
        if(handle.IsNull())
            return;

        gJobSystem->Wait(handle);
    }

    void Wait()
    {
        gJobSystem->Wait();
    }

    bool IsFinished(Handle<Job> handle)
    {
        return gJobSystem->IsFinished(handle);
    }

    void GarbageCollect()
    {
        gJobSystem->GarbageCollect();
    }
}