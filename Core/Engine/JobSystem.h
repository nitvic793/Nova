#pragma once

#include <Lib/Handle.h>

namespace nv
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
        virtual Handle<Job> Create(Job&& job) = 0;

    protected:
    };
}