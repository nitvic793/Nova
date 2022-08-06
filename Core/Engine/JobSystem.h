#pragma once

#include <Lib/Vector.h>

namespace nv::jobs
{
    class Job;

    class IJobSystem
    {
    public:
        virtual Handle<Job> Enqueue(Job&& job) = 0;
        virtual void        Wait(Handle<Job> job) = 0;
        virtual bool        IsFinished(Handle<Job> job) = 0;

        virtual ~IJobSystem() {}
    };

    extern IJobSystem* gJobSystem;

    void InitJobSystem(uint32_t threads);
    void DestroyJobSystem();

    Handle<Job> Execute(Job&& job);
    void        Wait(Handle<Job> handle);
    bool        IsFinished(Handle<Job> handle);
    
}