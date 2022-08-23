
#pragma warning( disable : 4530 ) // Disable warning related to exceptions

#include <NovaCore.h>
#include <Engine/Instance.h>
//#include <vld.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

#include <windows.h>

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
    nv::Instance instance("Test");
    if(instance.Init())
        instance.Run();

    auto size = nv::GetSystemAllocatedBytes();
    instance.Destroy();
    return 0;
}