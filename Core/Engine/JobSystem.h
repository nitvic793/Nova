#pragma once

#include <Lib/Vector.h>

namespace nv::jobs
{
    struct Job
    {
        using Fn = void(void*);
        Fn*     mFunction;
        void*   mArgs;

        Job(Fn* func):
            mFunction(func),
            mArgs(nullptr)
        {}

        Job(Fn* func, void* args):
            mFunction(func),
            mArgs(args)
        {}

        Job(const Job& job)
        {
            mFunction = job.mFunction;
            mArgs = job.mArgs;
        }

        Job():
            mFunction(nullptr),
            mArgs(nullptr)
        {}

        constexpr void Invoke() 
        { 
            mFunction(mArgs); 
        }
    };

    class IJobSystem
    {
    public:
        virtual Handle<Job> Enqueue(Job&& job) = 0;
        virtual void        Wait(Handle<Job> job) = 0;

    protected:
    };
}