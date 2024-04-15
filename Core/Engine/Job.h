#pragma once

#include <Lib/Vector.h>
#include <atomic>
#include <functional>

namespace nv::jobs
{
    class Job
    {
    public:
        using Fn = std::function<void(void*)>;

        Job(Fn func) :
            mFunction(func),
            mArgs(nullptr)
        {}

        Job(Fn&& func) :
            mFunction(std::move(func)),
            mArgs(nullptr)
        {}

        Job(Fn func, void* args) :
            mFunction(func),
            mArgs(args)
        {}

        Job(const Job& job)
        {
            mFunction = job.mFunction;
            mArgs = job.mArgs;
        }

        Job() :
            mFunction(nullptr),
            mArgs(nullptr)
        {}

        Job& operator=(const Fn& fn)
        {
            mFunction = fn;
            return *this;
        }

        Job& operator=(const Job& job)
        {
            mFunction = job.mFunction;
            mArgs = job.mArgs;
            mDependencies = job.mDependencies;
            mIsFinished.store(job.mIsFinished);
            return *this;
        }

        void Invoke()
        {
            if(mFunction)
                mFunction(mArgs);
        }

        bool IsFinished() const
        {
            return mIsFinished.load();
        }

        void SetFunction(Fn fn)
        {
            mFunction = fn;
        }

        constexpr void SetArgs(void* args)
        {
            mArgs = args;
        }

        constexpr void SetDependences(Span<Handle<Job>> dependencies)
        {
            mDependencies = dependencies;
        }

    protected:
        Fn mFunction;
        void* mArgs;
        std::atomic<bool>   mIsFinished = false;
        Span<Handle<Job>>   mDependencies = {};

        friend class JobSystem;
    };
}