#ifndef NV_PROFILER
#define NV_PROFILER

#pragma once

#define NV_PROFILING_USE_OPTICK 1
#define NV_PROFILING_USE_TRACY 0

#if NV_PROFILING_USE_OPTICK
#include <optick.h>
#endif

#if NV_PROFILING_USE_TRACY
#define TRACY_ON_DEMAND 1
#define TRACY_ENABLE 1
#include <tracy/Tracy.hpp>
#include <tracy/TracyD3D12.hpp>
#endif

#if _DEBUG || NV_ENABLE_PROFILING

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
#endif // NV_PROFILING_USE_OPTICK

#if NV_PROFILING_USE_TRACY
#define NV_APP(APP_NAME)    TracyAppInfo(APP_NAME, sizeof(APP_NAME))
#define NV_EVENT(scope)     ZoneNamedN(nvZone, scope, true)
#define NV_FRAME(name)      FrameMarkNamed(name)
#define NV_FRAME_MARK()     FrameMark
#define NV_THREAD(name)     tracy::SetThreadName(name)
#define NV_TAG(name, value) TracyPlot(name, value)

#define NV_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)    ((void)0)
#define NV_GPU_CONTEXT(COMMAND_LIST)                            ((void)0)
#define NV_GPU_EVENT(NAME)                                      ((void)0)
#define NV_GPU_FLIP(SWAP_CHAIN)                                 ((void)0)

#define NV_START_CAPTURE()      ((void)0)
#define NV_STOP_CAPTURE()       ((void)0)
#define NV_SAVE_CAPTURE(file)   ((void)0)
#define NV_SHUTDOWN()           ((void)0)
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

#endif // _DEBUG || NV_ENABLE_PROFILING

#endif // NV_PROFILER