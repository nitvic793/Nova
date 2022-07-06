
#pragma warning( disable : 4530 ) // Disable warning related to exceptions

#include <NovaCore.h>
#include <Engine/Log.h>
#include <Engine/System.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main()
{
    // Enabled memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    using namespace nv;
    nv::InitContext();
    nv::Info("Init {}", "Game");
    nv::Warn("Log Init");
    nv::SystemManager sysManager;
    nv::Error("Error");
    auto size = nv::GetSystemAllocatedBytes();
    nv::DestroyContext();
    return 0;
}