
#pragma warning( disable : 4530 ) // Disable warning related to exceptions

#include <NovaCore.h>
#include <Engine/Instance.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main()
{
    // Enabled memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    using namespace nv;
    nv::Instance instance("Test");
    if(instance.Init())
        instance.Run();

    instance.Destroy();

    auto size = nv::GetSystemAllocatedBytes();
    return 0;
}