
#pragma warning( disable : 4530 ) // Disable warning related to exceptions

#include "pch.h"

#include <Engine/Instance.h>

#if NV_ENABLE_VLD
#include <vld.h>
#endif

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

#include <windows.h>

#include "Systems/Register.h"

int main()
{
    nv::EnableLeakDetection();

    // Ensure "Current Directory" (relative path) is always the .exe's folder
    {
        char currentDir[1024] = {};
        GetModuleFileNameA(0, currentDir, 1024);
        char* lastSlash = strrchr(currentDir, '\\');
        if (lastSlash)
        {
            *lastSlash = 0;
            SetCurrentDirectoryA(currentDir);
        }
    }

    using namespace nv;
    nv::Instance mInstance("Test");

    // TODO: 
    // Handle KB + Mouse Input -> Create "InputSystem", Update DirectXTK KB + Mouse Static functions
    // Put game logic in App Project for now. 
    // Create Systems folder and create a simple ISystem instance and register using mInstance
    // Manually Create entities -> Attach Transforms -> Attach Renderable Components
    // Extra -> Find a way to draw Sponza
    if (mInstance.Init())
    {
        nv::RegisterGameSystems();
        mInstance.Run();
    }

    auto size = nv::GetSystemAllocatedBytes();
    mInstance.Destroy();
    return 0;
}