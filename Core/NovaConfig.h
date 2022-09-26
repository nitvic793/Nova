#ifndef NV_NOVACONFIG
#define NV_NOVACONFIG

#pragma once

#ifndef NV_PLATFORM_WINDOWS
#define NV_PLATFORM_WINDOWS 1
#endif  

#ifndef NV_RENDERER_DX12
#define NV_RENDERER_DX12 1
#endif  

#ifndef NV_RENDERER_ENABLE_DEBUG_LAYER
#define NV_RENDERER_ENABLE_DEBUG_LAYER 1
#endif  

#ifndef NV_ENABLE_PROFILING
#define NV_ENABLE_PROFILING 1
#endif

#ifndef NV_ENABLE_VLD
#define NV_ENABLE_VLD 0 // Enables Visual Leak Detector. Affects performance, only use for debugging.
#endif NV_ENABLE_VLD

#endif // !NV_NOVACONFIG
