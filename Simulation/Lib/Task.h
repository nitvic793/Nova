#pragma once

#include <Agent/Agent.h>

namespace nv::sim
{
    class ITask 
    {
    public:
        virtual void Run() = 0;
    };

    class TaskManager
    {
        
    };
}