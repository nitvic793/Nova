
//#include <vld.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <NovaCore.h>
#include "Memory/Memory.h"
#include <Memory/Allocator.h>
#include <Engine/System.h>
#include <Engine/JobSystem.h>
#include <Asset.h>
#include <AssetManager.h>
#include <Engine/Log.h>
#include <Math/Math.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/Window.h>
#include <Engine/JobSystem.h>
#include <Engine/Job.h>
#include <Engine/System.h>
#include <Engine/Timer.h>

#include <thread>
#include <Windows.h>

int main()
{
    // Enabled memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
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

    nv::InitContext(nullptr, "\\Data");
    auto assetManager = nv::asset::GetAssetManager();
    assetManager->ExportAssets(".\\Build\\Assets.novapkg");
    nv::DestroyContext();
}
