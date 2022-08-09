#include "pch.h"

#include <Engine/JobSystem.h>
#include <Engine/Job.h>

#include <Lib/Pool.h>
#include <Lib/ConcurrentQueue.h>

#include <Windows.h>

namespace nv::jobs
{
    IJobSystem* gJobSystem = nullptr;

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

        virtual Handle<Job> Enqueue(Job&& job) override
        {
            Handle<Job> handle = mJobs.Create();
            auto instance = mJobs.Get(handle);
            *instance = job;

            mQueue.Push(handle);
            mConditionVar.notify_one();
            return handle;
        }

        virtual void Wait(Handle<Job> handle) override
        {
            if (!mJobs.IsValid(handle)) 
                return;

            while (mJobs.IsValid(handle))
            {
                std::this_thread::yield();
            }
        }

        virtual void Wait() override
        {
            while (!mQueue.IsEmpty())
            {
                std::this_thread::yield();
            }
        }

        virtual bool IsFinished(Handle<Job> handle) override
        {
            return !mJobs.IsValid(handle);
        }

        void Stop()
        {
            mIsRunning = false;
            mConditionVar.notify_all(); // Unblock all threads and stop
            mJobs.Clear();
        }

        void Remove(Handle<Job> handle)
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mJobs.Remove(handle);
        }

        void Start()
        {
            mIsRunning = true;

            auto worker = [&]()
            {
                while (mIsRunning)
                {
                    {
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
                            {
                                std::unique_lock<std::mutex> lock(mMutex);
                                mJobs.Remove(jobHandle);
                            }
                        }
                    }
                }
            };

            for (uint32_t i = 0; i < mThreadCount; ++i)
            {
                mThreads[i] = std::thread(worker);

#ifdef _WIN32 // Credits: https://wickedengine.net/2018/11/24/simple-job-system-using-standard-c/#comments
                // Do Windows-specific thread setup:
                HANDLE handle = (HANDLE)mThreads[i].native_handle();

                // Put each thread on to dedicated core:
                DWORD_PTR affinityMask = 1ull << i;
                DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);

                HRESULT hr = SetThreadDescription(handle, L"Nova-Worker");
#endif // _WIN32

                mThreads[i].detach();
            }
        }

        ~JobSystem()
        {
            Wait();
            mJobs.Destroy();
        }

    private:
        uint32_t                        mThreadCount;
        bool                            mIsRunning;
        Pool<Job>                       mJobs;
        ConcurrentQueue<Handle<Job>>    mQueue;
        std::condition_variable         mConditionVar;
        std::mutex                      mMutex;
        std::vector<std::thread>        mThreads;
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

    Handle<Job> Execute(Job&& job)
    {
        return gJobSystem->Enqueue(std::move(job));
    }

    Handle<Job> Execute(void(*fn)(void*), void* args)
    {
        return gJobSystem->Enqueue({fn, args});
    }

    void Wait(Handle<Job> handle)
    {
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

}