#include "pch.h"

#include "AssetManager.h"
#include <Asset.h>
#include <AssetReload.h>

#include <Lib/Map.h>
#include <Lib/Pool.h>
#include <Lib/StringHash.h>
#include <Lib/ConcurrentQueue.h>

#include <Engine/JobSystem.h>
#include <Engine/Log.h>
#include <Engine/EventSystem.h>

#include <IO/Utility.h>
#include <IO/File.h>

#include <Types/MeshAsset.h>
#include <Types/ShaderAsset.h>
#include <Types/TextureAsset.h>
#include <Types/Serializers.h>
#include <Types/ConfigAsset.h>

#include <fstream>
#include <filesystem>
#include <mutex>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

namespace fs = std::filesystem;

#define NV_ASSET_DEBUG_LOADER 1

namespace nv::asset
{
    static const char* gpDataPath = nullptr;
    IAssetManager* gpAssetManager = nullptr;

    constexpr const char MESH_PATH[]            = "Mesh";
    constexpr const char TEXTURES_PATH[]        = "Textures";
    constexpr const char SHADERS_PATH[]         = "Shaders";
    constexpr const char CONFIGS_PATH[]         = "Configs";
    constexpr const char PACKAGE_EXTENSION[]    = ".novapkg";

    constexpr bool HOT_RELOAD_ENABLED = true;

    constexpr bool StringContains(const std::string& str, const std::string& inString) 
    {
        return inString.find(str) != std::string::npos;  
    }

    constexpr AssetType GetAssetType(const char* pathString)
    {
        if (StringContains(MESH_PATH, pathString))      return ASSET_MESH;
        if (StringContains(TEXTURES_PATH, pathString))  return ASSET_TEXTURE;
        if (StringContains(SHADERS_PATH, pathString))   return ASSET_SHADER;
        if (StringContains(CONFIGS_PATH, pathString))   return ASSET_CONFIG;

        return ASSET_INVALID;
    }

    std::string GetNormalizedBuildPath(const std::string& path)
    {
        const char* replaceEmpty = "";
        std::string_view dataPath = gpDataPath;
        dataPath.remove_prefix(1);
        std::string outPath = path;
        auto pos = outPath.find(dataPath);
        if(pos != std::string::npos)
            outPath = outPath.replace(pos, dataPath.size() + 1, "");
        io::NormalizePath(outPath);
        return outPath;
    }

    class AssetManager : public IAssetManager
    {
        struct CallbackData
        {
            AssetLoadCallback   mCallback;
            Asset*              mAsset;
        };

    public:
        virtual void Init(const char* assetPath) override
        {
            mAssets.Init();
            fs::path path = fs::current_path().string() + assetPath;
            for (const auto& entry : fs::recursive_directory_iterator(path))
            {
                if (!fs::is_regular_file(entry.path()))
                    continue;

                const auto path = entry.path().string();
                const auto relative = GetNormalizedBuildPath(fs::relative(path, fs::current_path()).string());

                if (path.find(PACKAGE_EXTENSION) != std::string::npos)
                {
                    mPackageFiles.push_back(path);
                    LoadPackageFile(path.c_str());
                    continue;
                }

                const AssetID id = { GetAssetType(relative.c_str()), ID(relative.c_str())};
                auto handle = mAssets.Create();
                Asset* asset = mAssets.Get(handle);
                asset->Set(id, {});
                asset->SetState(STATE_UNLOADED);
                mAssetMap[id.mId] = handle;
#if NV_ASSET_DEBUG_LOADER
                mAssetPathMap[id.mId] = entry.path().string();
#endif
            }
        }

        virtual Asset* GetAsset(AssetID id) const override
        {
            auto it = mAssetMap.find(id);
            if (it != mAssetMap.end())
                return mAssets.Get(it->second);

            return nullptr;
        }

        virtual Asset* GetAsset(Handle<Asset> asset) const override
        {
            return mAssets.Get(asset);
        }

        void LoadPackageFile(const char* path)
        {
            std::ifstream file(path, std::ios::binary);
            uint32_t assetCount = 0;
            auto fileSize = io::GetFileSize(path);
            uint64_t totalBytesRead = 0;
            uint32_t assetsLoaded = 0;
            {
                cereal::BinaryInputArchive archive(file);
                archive(assetCount);
            }

            while (!file.eof() && file.good())
            {
                Handle<Asset> handle;
                auto asset = mAssets.CreateInstance(handle);
                totalBytesRead += ImportAsset(file, asset);
                mAssetMap[asset->GetID()] = handle;
                if (asset->GetState() == STATE_LOADED)
                {
                    mAssetMap[asset->GetID()] = handle;
                    if (asset->GetType() == ASSET_CONFIG)
                    {
                        const auto hash = asset->GetHash();
                        if (hash == ID("Configs/ShaderConfig.json"))
                        {
                            auto data = asset->GetAssetData();
                            nv::io::MemoryStream stream((const char*)data.mData, data.mSize);
                            LoadShaderConfigDataBinary(stream);
                        }
                        else if (hash == ID("Configs/Materials.json"))
                        {
                            auto data = asset->GetAssetData();
                            nv::io::MemoryStream stream((const char*)data.mData, data.mSize);
                            Load<MaterialDatabase, SERIAL_BINARY>(stream, "Materials");
                        }
                    }
                }
                else
                {
                    log::Error("[Asset] Unable to load asset package file");
                    return;
                }

                assetsLoaded++;
                if (assetsLoaded == assetCount)
                    break;
            }
        }

        bool LoadAssetFromFile(Asset* asset, const char* path)
        {
            size_t size = io::GetFileSize(path);
            Byte* pBuffer = (Byte*)Alloc(size);

            asset->SetState(STATE_LOADING);
            AssetData data = { size, pBuffer };
            bool result = io::ReadFile(path, data.mData, (uint32_t)size);
            asset->SetData(data);
            asset->SetState(result ? STATE_LOADED : STATE_ERROR);
            return result;
        }

        bool LoadAssetFromFile(Asset* asset, bool isTextFile = false)
        {
            const auto& path = mAssetPathMap[asset->GetID()];
            return LoadAssetFromFile(asset, path.c_str());
        }

        virtual Handle<Asset> LoadAsset(AssetID id, AssetLoadCallback callback, bool wait) override
        {
#if NV_ASSET_DEBUG_LOADER
            auto it = mAssetMap.find(id.mId);
            if (it != mAssetMap.end())
            {
                Asset* asset = mAssets.Get(it->second);
                if(!asset)
                    return Null<Asset>();

                if (asset->GetState() != STATE_LOADED && asset->GetState() != STATE_ERROR)
                {
                    const auto& path = mAssetPathMap[id.mId];
                    size_t size = io::GetFileSize(path.c_str());
                    Byte* pBuffer = (Byte*)Alloc(size);

                    auto loadAsset = [&](void* ctx)
                    {
                        asset->SetState(STATE_LOADING);
                        AssetData data = { size, pBuffer };
                        bool result = io::ReadFile(path.c_str(), data.mData, (uint32_t)size);
                        asset->Set(id, data);
                        asset->SetState(result ? STATE_LOADED : STATE_ERROR);
                        if (result)
                            log::Info("[Asset] Load {}: OK", path.c_str());
                        else
                            log::Error("[Asset] Load {}: ERROR", path.c_str());

                        if (callback)
                            mCallbacks.Push({ callback, asset });
                    };

                    if (wait)
                        loadAsset(nullptr);
                    else
                        jobs::Execute(loadAsset);
                }

                return it->second;
            }
#endif
            return Null<Asset>();
        }

        virtual void UnloadAsset(Handle<Asset> asset) override
        {
            Asset* pAsset = mAssets.Get(asset);
            if (pAsset)
            {
                if (pAsset->GetState() == STATE_LOADED)
                {
                    Free(pAsset->GetData());
                    pAsset->SetData({ 0, nullptr });
                    pAsset->SetState(STATE_UNLOADED);
                    return;
                }
            }

            assert(false);
            const auto& assetPath = mAssetPathMap[pAsset->GetID()];
            log::Error("[Asset] Error unloading asset {}", assetPath.c_str());
        }

        virtual void UnloadAsset(AssetID asset) override
        {
            auto handle = mAssetMap[asset.mId];
            UnloadAsset(handle);
        }

        virtual Handle<jobs::Job> ExportAssets(const char* exportPath) override
        {
            auto folder = fs::path(exportPath).parent_path();
            if (!fs::is_directory(folder))
                fs::create_directory(folder);

            auto handle = jobs::Execute([&](void* ctx) 
            {
                std::ofstream file(exportPath, std::ios::binary | std::ios::trunc);
                if (!file.is_open() || file.bad())
                {
                    log::Error("[Asset] Unable to open file to export assets.");
                    return;
                }

                {
                    cereal::BinaryOutputArchive archive(file);
                    uint32_t assetCount = mAssets.Size();
                    archive(assetCount);
                }

                // Export configs first
                for (auto item : mAssetMap)
                {
                    Asset* asset = mAssets.Get(item.second);
                    if (asset->GetType() == ASSET_CONFIG)
                    {
                        if (asset)
                        {
                            LoadAssetFromFile(asset);
                            ExportAsset(asset, file); // TODO: Export to file. 
                            UnloadAsset(item.second);
                        }
                    }
                }

                for (auto item : mAssetMap)
                {
                    Asset* asset = mAssets.Get(item.second);
                    if (asset->GetType() == ASSET_CONFIG)
                        continue;

                    if (asset)
                    {
                        LoadAssetFromFile(asset);
                        ExportAsset(asset, file); // TODO: Export to file. 
                        UnloadAsset(item.second);
                    }
                }
            });

            return handle;
        }

        void DeallocateAsset(Asset* asset)
        {
            if (asset)
                Free(asset->GetData());

            asset->SetBuffer(nullptr, 0);
        }

        void CleanUp()
        {
            for (auto& item : mAssetMap)
            {
                Asset* asset = mAssets.Get(item.second);
                DeallocateAsset(asset);
            }
        }

        virtual void Tick() override
        {
            while (!mCallbacks.IsEmpty())
            {
                auto callback = mCallbacks.Pop();
                callback.mCallback(callback.mAsset);
            }
        }

        virtual void Reload(const char* file) override
        {
            if constexpr (!HOT_RELOAD_ENABLED)
                return;

            std::string_view path(file);
            path.remove_prefix(_countof(RAW_ASSET_PATH) - 1);
            AssetType assetType = GetAssetType(path.data());
            AssetID id = { assetType, ID(path.data()) };

            auto asset = GetAsset(id);
            DeallocateAsset(asset);
            if(assetType == ASSET_SHADER)
            {
                size_t size = io::GetFileSize(file);
                Byte* pBuffer = (Byte*)Alloc(size);
                AssetData data = { size, pBuffer };
                bool result = io::ReadFile(file, data.mData, (uint32_t)size);

                std::ostringstream sstream;
                ShaderAsset shader;
                shader.Export(data, path.data(), sstream);
                Free(pBuffer);

                auto buffer = sstream.str();
                if (buffer.empty())
                {
                    log::Error("[Asset] Error reloading asset {}", path.data());
                    return;
                }

                pBuffer = (Byte*)Alloc(buffer.size());
                memcpy(pBuffer, buffer.c_str(), buffer.size());
                data = { buffer.size(), pBuffer };

                asset->SetData(data);

                AssetReloadEvent event;
                event.mAssetId = id;
                gEventBus.Publish(&event);
                log::Info("[Asset] Reloaded {}", path.data());
            }
        }

        ~AssetManager()
        {
            CleanUp();
            mAssets.Destroy();
        }

    protected:
        void ExportAsset(Asset* asset, std::ostream& ostream)
        {
            cereal::BinaryOutputArchive archive(ostream);
            auto path = GetNormalizedBuildPath(fs::relative(mAssetPathMap[asset->GetID()], fs::current_path()).string());

            const auto writeHeader = [asset, &archive, this, &path](size_t size)
            {
                Header header = { .mAssetId = asset->GetAssetID(), .mSizeBytes = size };
                archive(header);
                archive(path);
            };

            switch (asset->GetType())
            {
            case ASSET_MESH:
            {
                MeshAsset mesh;
                std::ostringstream sstream;
                mesh.Export(asset->GetAssetData(), sstream);
                writeHeader((size_t)sstream.tellp());
                ostream.write(sstream.str().c_str(), sstream.str().size());
                break;
            }
            case ASSET_SHADER:
            {
                const auto& data = asset->GetAssetData();
                std::ostringstream sstream;
                ShaderAsset shader;
                shader.Export(data, path.c_str(), sstream); // Depends on shader config which should be loaded first.
                writeHeader((size_t)sstream.tellp());
                ostream.write(sstream.str().c_str(), sstream.str().size());
                break;
            }
            case ASSET_TEXTURE:
            {
                const auto& data = asset->GetAssetData();
                std::ostringstream sstream;
                TextureAsset texture;
                TextureAsset::Type type = TextureAsset::WIC;
                if (path.find(".dds") != std::string::npos)
                    type = TextureAsset::DDS;

                texture.Export(data, sstream, type);
                writeHeader((size_t)sstream.tellp());
                ostream.write(sstream.str().c_str(), sstream.str().size());
                break;
            }
            case ASSET_CONFIG:
            {
                std::ostringstream sstream;
                if (path.find("ShaderConfig") != std::string::npos)
                {
                    auto data = asset->GetAssetData();
                    nv::io::MemoryStream stream((const char*)data.mData, data.mSize);
                    LoadShaderConfigData(stream);
                    ExportShaderConfigDataBinary(sstream);
                }
                else if (path.find("Materials") != std::string::npos)
                {
                    auto data = asset->GetAssetData();
                    nv::io::MemoryStream stream((const char*)data.mData, data.mSize);
                    Load<MaterialDatabase>(stream, "Materials");
                    Export<MaterialDatabase, SERIAL_BINARY>(sstream, "Materials");
                }

                writeHeader((size_t)sstream.tellp());
                ostream.write(sstream.str().c_str(), sstream.str().size());
                break;
            }
            }

            log::Info("[Asset] Exported : {}", path.c_str());
        }

        size_t ImportAsset(std::istream& istream, Asset* pAsset)
        {
            // TODO:
            // Store offsets instead and read assets on-demand
            auto curPos = istream.tellg();
            cereal::BinaryInputArchive archive(istream);
            Header header = {};
            std::string name;
            size_t size = 0;

            archive(header);
            archive(name);
           
            if (header.mSizeBytes != 0)
            {
                void* pBuffer = Alloc(header.mSizeBytes);
                //istream.seekg(header.mSizeBytes, std::ios::cur);
                istream.read((char*)pBuffer, header.mSizeBytes); // TODO: Seek forward mSizeBytes and store istream.tellg() offset in a map
                header.mOffset = istream.tellg();
                pAsset->Set(header.mAssetId, { header.mSizeBytes, (uint8_t*)pBuffer });
                pAsset->SetState(STATE_LOADED);
                log::Info("[Asset] Loaded from package: {}", name.c_str());
                mAssetPathMap[pAsset->GetID()] = name;
                size = istream.tellg() - curPos;
            }
            return size;
        }

    protected:
        Pool<Asset>                         mAssets;
        HashMap<uint64_t, Handle<Asset>>    mAssetMap;
#if NV_ASSET_DEBUG_LOADER
        HashMap<uint64_t, std::string>      mAssetPathMap;
#endif
        std::mutex                          mMutex;
        std::vector<std::string>            mPackageFiles;
        ConcurrentQueue<CallbackData>       mCallbacks;
    };

    void InitAssetManager(const char* assetPath)
    {
        gpAssetManager = Alloc<AssetManager>();
        gpDataPath = assetPath;
        gpAssetManager->Init(assetPath);
    }

    void DestroyAssetManager()
    {
        Free<AssetManager>((AssetManager*)gpAssetManager);
    }

    IAssetManager* GetAssetManager()
    {
        return gpAssetManager;
    }

    class AssetPackage
    {
    public:
        AssetPackage(const char* path);

        void Load();
        void LoadAsset(AssetID asset, AssetLoadCallback callback, bool async = true);
        void UnloadAsset(AssetID asset);

    private:
        std::string                 mPath;
        HashMap<uint64_t, Header>   mAssetHeaderMap;
    };
}
