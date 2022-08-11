#pragma once

#include <Lib/Vector.h>
#include <Engine/Job.h>

namespace nv::jobs
{
    class Job;

    class IJobSystem
    {
    public:
        virtual Handle<Job> Enqueue(Job&& job) = 0;
        virtual void        Wait(Handle<Job> job) = 0;
        virtual void        Wait() = 0;
        virtual bool        IsFinished(Handle<Job> job) = 0;

        virtual ~IJobSystem() {}
    };

    extern IJobSystem* gJobSystem;

    void InitJobSystem(uint32_t threads);
    void DestroyJobSystem();

    Handle<Job> Execute(Job::Fn&& job);
    void        Wait(Handle<Job> handle);
    void        Wait();
    bool        IsFinished(Handle<Job> handle);
    
}