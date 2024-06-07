#ifndef NV_PROFILER
#define NV_PROFILER

#pragma once

#if _DEBUG || NV_ENABLE_PROFILING

#define NV_MAX_CALLSTACK_DEPTH 32

#define NV_PROFILING_USE_OPTICK 0
#define NV_PROFILING_USE_TRACY 1

#if NV_PROFILING_USE_OPTICK
#include <optick.h>
#endif

#if NV_PROFILING_USE_TRACY

#define TRACY_ENABLE 1
#define TRACY_CALLSTACK 1
#define TRACY_ON_DEMAND 0

#include <tracy/Tracy.hpp>
#include <tracy/TracyD3D12.hpp>
#endif

#if NV_PROFILING_USE_OPTICK
#define NV_APP(APP_NAME)    OPTICK_APP(APP_NAME)
#define NV_EVENT(scope)     OPTICK_EVENT(scope)
#define NV_FRAME(name)      OPTICK_FRAME(name)
#define NV_THREAD(name)     OPTICK_THREAD(name)
#define NV_TAG(name, value) OPTICK_TAG(name, value)
#define NV_FRAME_MARK()     ((void)0)

#define NV_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)    OPTICK_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)
#define NV_GPU_CONTEXT(COMMAND_LIST)                            OPTICK_GPU_CONTEXT(COMMAND_LIST)
#define NV_GPU_EVENT(NAME)                                      OPTICK_GPU_EVENT(NAME)
#define NV_GPU_FLIP(SWAP_CHAIN)                                 OPTICK_GPU_FLIP(SWAP_CHAIN)

#define NV_START_CAPTURE()      OPTICK_START_CAPTURE()
#define NV_STOP_CAPTURE()       OPTICK_STOP_CAPTURE()
#define NV_SAVE_CAPTURE(file)   OPTICK_SAVE_CAPTURE(file)
#define NV_SHUTDOWN()           OPTICK_SHUTDOWN() 

#define NV_MEM_ALLOC(ptr, size)             ((void)0)
#define NV_MEM_ALLOCN(ptr, size, name)      ((void)0)
#define NV_MEM_FREE(ptr)                    ((void)0)
#define NV_MEM_FREEN(ptr, name)             ((void)0)

#endif // NV_PROFILING_USE_OPTICK

#if NV_PROFILING_USE_TRACY

namespace nv
{
    struct TracyGlobalInfo
    {
        static tracy::D3D12QueueCtx* mD3D12Ctx;
    };
}

#define NV_APP(APP_NAME)    TracyAppInfo(APP_NAME, sizeof(APP_NAME))
#define NV_EVENT(scope)     ZoneScopedN(scope)
#define NV_FRAME(name)      FrameMarkNamed(name)
#define NV_FRAME_MARK()     FrameMark
#define NV_THREAD(name)     tracy::SetThreadName(name) 
#define NV_TAG(name, value) TracyPlot(name, value)

#define NV_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)    nv::TracyGlobalInfo::mD3D12Ctx = TracyD3D12Context(DEVICE, *CMD_QUEUES)
#define NV_GPU_CONTEXT(COMMAND_LIST)                            ((void)0)
#define NV_GPU_EVENT(NAME, COMMAND_LIST)                        ((void)0)//TracyD3D12Zone(nv::TracyGlobalInfo::mD3D12Ctx, COMMAND_LIST, NAME)
#define NV_GPU_FLIP(SWAP_CHAIN)                                 ((void)0)//TracyD3D12Collect(nv::TracyGlobalInfo::mD3D12Ctx)

#define NV_START_CAPTURE()      ((void)0)
#define NV_STOP_CAPTURE()       ((void)0)
#define NV_SAVE_CAPTURE(file)   ((void)0)
#define NV_SHUTDOWN()           TracyD3D12Destroy(nv::TracyGlobalInfo::mD3D12Ctx)

#define NV_MEM_ALLOC(ptr, size)             TracyAllocS(ptr, size, NV_MAX_CALLSTACK_DEPTH)
#define NV_MEM_ALLOCN(ptr, size, name)      TracyAllocNS(ptr, size, NV_MAX_CALLSTACK_DEPTH, name)
#define NV_MEM_FREE(ptr)                    TracyFreeS(ptr, NV_MAX_CALLSTACK_DEPTH)
#define NV_MEM_FREEN(ptr, name)             TracyFreeNS(ptr, NV_MAX_CALLSTACK_DEPTH, name)

#define NV_MUTEX(type, name)                TracyLockable(type, name)
#define NV_LOCKABLE(type)                   LockableBase(type)

#endif // NV_PROFILING_USE_TRACY

#else // _DEBUG || NV_ENABLE_PROFILING

#define NV_EVENT()         
#define NV_EVENT(scope)    
#define NV_THREAD(name)    
#define NV_TAG(name, value)
#define NV_FRAME(name)  
#define NV_FRAME_MARK()     ((void)0)

#define NV_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)  
#define NV_GPU_CONTEXT(COMMAND_LIST)                          
#define NV_GPU_EVENT(NAME)                                    
#define NV_GPU_FLIP(SWAP_CHAIN)                               

#define NV_START_CAPTURE()   
#define NV_STOP_CAPTURE()   
#define NV_SAVE_CAPTURE()   
#define NV_SAVE_CAPTURE(file) 

#define NV_MEM_ALLOC(ptr, size)             ((void)0)
#define NV_MEM_ALLOCN(ptr, size, name)      ((void)0)
#define NV_MEM_FREE(ptr)                    ((void)0)
#define NV_MEM_FREEN(ptr, name)             ((void)0)

#endif // _DEBUG || NV_ENABLE_PROFILING

#endif // NV_PROFILER