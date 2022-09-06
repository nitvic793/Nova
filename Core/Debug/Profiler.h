#ifndef NV_PROFILER
#define NV_PROFILER

#pragma once

#include <optick.h>

#if _DEBUG || NV_ENABLE_PROFILING

#define NV_APP(APP_NAME)    OPTICK_APP(APP_NAME)
#define NV_EVENT(scope)     OPTICK_EVENT(scope)
#define NV_FRAME(name)      OPTICK_FRAME(name)
#define NV_THREAD(name)     OPTICK_THREAD(name)
#define NV_TAG(name, value) OPTICK_TAG(name, value)

#define NV_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)    OPTICK_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)
#define NV_GPU_CONTEXT(COMMAND_LIST)                            OPTICK_GPU_CONTEXT(COMMAND_LIST)
#define NV_GPU_EVENT(NAME)                                      OPTICK_GPU_EVENT(NAME)
#define NV_GPU_FLIP(SWAP_CHAIN)                                 OPTICK_GPU_FLIP(SWAP_CHAIN)

#define NV_START_CAPTURE()      OPTICK_START_CAPTURE()
#define NV_STOP_CAPTURE()       OPTICK_STOP_CAPTURE()
#define NV_SAVE_CAPTURE(file)   OPTICK_SAVE_CAPTURE(file)
#define NV_SHUTDOWN()           OPTICK_SHUTDOWN() 

#else // _DEBUG || NV_ENABLE_PROFILING

#define NV_EVENT()         
#define NV_EVENT(scope)    
#define NV_THREAD(name)    
#define NV_TAG(name, value)

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