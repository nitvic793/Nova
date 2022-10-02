
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
#include <Types/ShaderAsset.h>
#include <Engine/Log.h>
#include <Math/Math.h>
#include <Renderer/Renderer.h>
#include <Renderer/Device.h>
#include <Renderer/Window.h>
#include <Engine/JobSystem.h>
#include <Engine/Job.h>
#include <Engine/System.h>
#include <Engine/Timer.h>
#include <Engine/Log.h>
#include <combaseapi.h>

#include <thread>
#include <Windows.h>
#include <fstream>

#include <Configs/MaterialDatabase.h>
#include <Types/ConfigAsset.h>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

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
    
    //nv::asset::PBRMaterial bronzeMaterial =
    //{
    //    .mAlbedoTexture = { nv::asset::ASSET_TEXTURE, nv::ID("Textures/bronze_albedo.png")},
    //    .mNormalTexture = { nv::asset::ASSET_TEXTURE, nv::ID("Textures/bronze_normals.png")},
    //    .mRoughnessTexture = { nv::asset::ASSET_TEXTURE, nv::ID("Textures/bronze_roughness.png")},
    //    .mMetalnessTexture = { nv::asset::ASSET_TEXTURE, nv::ID("Textures/bronze_metal.png")},
    //};

    //{
    //    std::ifstream file("test.json", std::ios::binary);
    //    nv::asset::Load<nv::asset::MaterialDatabase, nv::asset::SERIAL_JSON>(file, "MaterialsTest");
    //}

    //nv::asset::gMaterialDatabase.mMaterials["Floor"] = bronzeMaterial;
    //nv::asset::gMaterialDatabase.mMaterials["Bronze"] = bronzeMaterial;

    //{
    //    std::ofstream file("test.json", std::ios::binary);
    //    nv::asset::Export<nv::asset::MaterialDatabase, nv::asset::SERIAL_JSON>(file, "MaterialsTest");
    //}

    constexpr int PIPELINE_ERROR = -1;
    constexpr int PIPELINE_SUCCESS = 0;
    auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        nv::log::Error("[Asset] CoInitialize Failure. Aborting asset export.");
        return PIPELINE_ERROR;
    }

    nv::InitContext(nullptr, "\\Data");
    auto assetManager = nv::asset::GetAssetManager();
    assetManager->ExportAssets(".\\Build\\Assets.novapkg");
    nv::DestroyContext();

    CoUninitialize();

    return PIPELINE_SUCCESS;
}
