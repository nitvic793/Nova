#pragma once

#include <Lib/Vector.h>
#include <atomic>

namespace nv::jobs
{
    class Job
    {
    public:
        using Fn = void(*)(void*);

        constexpr Job(Fn func) :
            mFunction(func),
            mArgs(nullptr)
        {}

        constexpr Job(Fn func, void* args) :
            mFunction(func),
            mArgs(args)
        {}

        constexpr Job(const Job& job)
        {
            mFunction = job.mFunction;
            mArgs = job.mArgs;
        }

        constexpr Job() :
            mFunction(nullptr),
            mArgs(nullptr)
        {}

        constexpr Job& operator=(void (*fn)(void*))
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

        constexpr void Invoke()
        {
            if(mFunction)
                mFunction(mArgs);
        }

        bool IsFinished() const
        {
            return mIsFinished.load();
        }

        constexpr void SetFunction(Fn fn)
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