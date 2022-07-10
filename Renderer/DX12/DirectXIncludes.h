#pragma once

#include <NovaCore.h>

#if NV_RENDERER_DX12 && NV_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN   

#include <windows.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include <d3d12.h>
#include <d3dCompiler.h>

#endif